#include "main_window.h"

#pragma warning(push, 1)
#include <array>
#include <sstream>
#include <regex>
#include <boost/lexical_cast.hpp>
#include <boost/signals2.hpp>
#include <boost/timer/timer.hpp>
#include <boost/algorithm/clamp.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/from_settings.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QMenuBar>
#include <QFileDialog>
#include <QStatusBar>
#include <QLabel>
#include <QToolBar>
#include <QLineEdit>
#include <QComboBox>
#include <QCoreApplication>
#include <QApplication>
#include <QSpinBox>
#include <QDateTime>
#include <QMessageBox>
#include <QHostInfo>
#include <QXmlStreamReader>
#pragma warning(pop)

#include "gamelib/nlhe_state.h"
#include "cfrlib/strategy.h"
#include "util/card.h"
#include "cfrlib/nlhe_strategy.h"
#include "abslib/holdem_abstraction.h"
#include "util/random.h"
#include "holdem_strategy_widget.h"
#include "table_manager.h"
#include "input_manager.h"
#include "state_widget.h"
#include "qt_log_sink.h"
#include "fake_window.h"
#include "smtp.h"
#include "site_settings.h"
#include "util/version.h"
#include "captcha_manager.h"
#include "window_manager.h"

#define ENSURE(x) ensure(x, #x, __LINE__)

namespace
{
    typedef std::map<QDate, std::vector<std::pair<QDateTime, QDateTime>>> spans_t;

    template<class T>
    typename T::const_iterator find_nearest(const T& map, const typename T::key_type& value)
    {
        if (map.empty())
            return map.end();

        const auto it = map.lower_bound(value);
        return it != map.end() ? it : boost::prior(map.end());
    }

    QString secs_to_hms(std::int64_t seconds_)
    {
        auto seconds = static_cast<int>(seconds_);

        const auto hours = static_cast<int>(seconds / 3600);
        seconds -= hours * 3600;

        const auto minutes = static_cast<int>(seconds / 60);
        seconds -= minutes * 60;

        return QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    }

    bool is_schedule_active(const spans_t& daily_spans, QDateTime* time)
    {
        const auto now = QDateTime::currentDateTime();
        const auto spans = daily_spans.lower_bound(now.date());

        if (spans != daily_spans.end())
        {
            for (const auto span : spans->second)
            {
                const auto first = span.first;
                const auto second = span.second;

                if (now < first)
                {
                    *time = first;
                    return false;
                }
                else if (now >= first && now < second)
                {
                    *time = second;
                    return true;
                }
            }

            const auto next = boost::next(spans);

            if (next != daily_spans.end() && !next->second.empty())
            {
                *time = next->second.front().first;
                return false;
            }
        }

        *time = QDateTime();

        return false;
    }

    QString make_schedule_string(const bool active, const QDateTime& next_activity)
    {
        return QString("Next %1 at %3 (%2)").arg(active ? "break" : "play")
            .arg(secs_to_hms(QDateTime::currentDateTime().secsTo(next_activity)))
            .arg(next_activity.toString(Qt::ISODate));
    }

    spans_t read_schedule_file(const std::string& filename)
    {
        QFile file(filename.c_str());

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return spans_t();

        spans_t activity_spans;
        QXmlStreamReader reader(&file);

        while (reader.readNextStartElement())
        {
            if (reader.name() == "schedule")
            {
                // do nothing
            }
            else if (reader.name() == "span")
            {
                const auto day = QDate::fromString(reader.attributes().value("id").toString(), "yyyy-MM-dd");

                for (const auto& i : reader.attributes().value("value").toString().split(","))
                {
                    if (i == "0")
                        continue;

                    const auto j = i.split(QChar('-'));

                    if (j.size() != 2)
                    {
                        BOOST_LOG_TRIVIAL(error) << "Invalid activity span setting";
                        continue;
                    }

                    activity_spans[day].push_back(std::make_pair(
                        QDateTime(day, QTime::fromString(j.at(0), "HH:mm:ss")),
                        QDateTime(day, QTime::fromString(j.at(1), "HH:mm:ss"))));
                }

                reader.skipCurrentElement();
            }
            else
            {
                reader.skipCurrentElement();
            }
        }

        return activity_spans;
    }

    struct qt_sink_factory : public boost::log::sink_factory<char>
    {
        qt_sink_factory(QPlainTextEdit& log) : log_(log) {}

        boost::shared_ptr<boost::log::sinks::sink> qt_sink_factory::create_sink(const settings_section& settings)
        {
            typedef boost::log::sinks::synchronous_sink<qt_log_sink> sink_t;
            boost::shared_ptr<qt_log_sink> sink_backend(new qt_log_sink(&log_));
            boost::shared_ptr<sink_t> sink_frontend(new sink_t(sink_backend));

            if (const auto p = settings["Format"])
                sink_frontend->set_formatter(boost::log::parse_formatter(*p.get()));

            return sink_frontend;
        }

        QPlainTextEdit& log_;
    };

    template<class T>
    T round_multiple(const T val, const T mul)
    {
        if (mul == 0)
            return val;

        return static_cast<T>(std::round(static_cast<double>(val) / mul) * mul);
    }
}

main_window::main_window(const std::string& settings_path)
    : engine_(std::random_device()())
    , input_manager_(new input_manager)
    , schedule_active_(false)
    , settings_(new site_settings)
    , smtp_(nullptr)
    , captcha_manager_(new captcha_manager)
    , window_manager_(new window_manager)
    , error_allowance_(0)
    , error_time_(QDateTime::currentDateTime())
    , max_error_count_(0)
{
    strategy_widget_ = new holdem_strategy_widget(this, Qt::Tool);
    strategy_widget_->setVisible(false);
    state_widget_ = new state_widget(this, Qt::Tool);
    state_widget_->setVisible(false);
    connect(state_widget_, &state_widget::board_changed, this, &main_window::state_widget_board_changed);
    connect(state_widget_, &state_widget::state_changed, this, &main_window::state_widget_state_changed);
    connect(state_widget_, &state_widget::stack_changed, [this](const double val)
    {
        state_widget_->set_strategy(!strategies_.empty()
            ? find_nearest(strategies_, static_cast<int>(val))->second.get()
            : nullptr);
    });

    auto toolbar = addToolBar("File");
    toolbar->setMovable(false);
    open_action_ = toolbar->addAction(QIcon(":/icons/folder_page_white.png"), "&Open...");
    open_action_->setIconText("Open...");
    open_action_->setToolTip("Open...");
    connect(open_action_, &QAction::triggered, this, &main_window::open_strategy);
    auto action = toolbar->addAction(QIcon(":/icons/map.png"), "Show strategy");
    connect(action, &QAction::triggered, this, &main_window::show_strategy_changed);
    action = toolbar->addAction(QIcon(":/icons/chart_organisation.png"), "Modify state");
    connect(action, &QAction::triggered, this, &main_window::modify_state_changed);
    save_images_ = toolbar->addAction(QIcon(":/icons/picture_save.png"), "Save images");
    save_images_->setCheckable(true);
    toolbar->addSeparator();

    title_filter_ = new QLineEdit(this);
    title_filter_->setPlaceholderText("Window title");
    toolbar->addWidget(title_filter_);
    toolbar->addSeparator();

    schedule_action_ = toolbar->addAction(QIcon(":/icons/time.png"), "Schedule");
    schedule_action_->setCheckable(true);
    connect(schedule_action_, &QAction::toggled, this, &main_window::schedule_changed);

    autoplay_action_ = toolbar->addAction(QIcon(":/icons/control_play.png"), "Autoplay");
    autoplay_action_->setCheckable(true);
    connect(autoplay_action_, &QAction::toggled, this, &main_window::autoplay_changed);

    log_ = new QPlainTextEdit(this);
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(1000);

    setCentralWidget(log_);

    capture_timer_ = new QTimer(this);
    connect(capture_timer_, &QTimer::timeout, this, &main_window::capture_timer_timeout);

    site_label_ = new QLabel(this);
    statusBar()->addWidget(site_label_, 1);
    strategy_label_ = new QLabel(this);
    statusBar()->addWidget(strategy_label_, 1);
    schedule_label_ = new QLabel(this);
    statusBar()->addWidget(schedule_label_, 1);
    update_statusbar();

    boost::log::add_common_attributes();
    boost::log::register_sink_factory("Window", boost::make_shared<qt_sink_factory>(*log_));
    boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");

    load_settings(settings_path);

    BOOST_LOG_TRIVIAL(info) << "Starting capture";

    capture_timer_->start(1000);
    mark_time_.start();

    setWindowTitle(QString("gui %1").arg(util::GIT_VERSION));
}

main_window::~main_window()
{
    BOOST_LOG_TRIVIAL(info) << "Closing...";
}

void main_window::capture_timer_timeout()
{
    try
    {
        if (const auto p = settings_->get_number("mark"))
        {
            if (mark_time_.elapsed() >= static_cast<int>(*p * 1000.0))
            {
                mark_time_.restart();
                BOOST_LOG_TRIVIAL(info) << "-- MARK --";
            }
        }

        if (!schedule_action_->isChecked() && QFileInfo("enable.txt").exists())
        {
            BOOST_LOG_TRIVIAL(info) << "enable.txt found, enabling schedule";
            schedule_action_->setChecked(true);
        }
        else if (schedule_action_->isChecked() && QFileInfo("disable.txt").exists())
        {
            BOOST_LOG_TRIVIAL(info) << "disable.txt found, disabling schedule";
            schedule_action_->setChecked(false);
        }

        handle_schedule();
        update_capture();

        for (int i = 0; i < table_windows_.size(); ++i)
        {
            const auto& window = table_windows_[i];

            if (!window->is_valid())
                continue;

            try
            {
                process_snapshot(i, *window);
            }
            catch (const std::exception& e)
            {
                handle_error(e);
            }
        }

        check_idle(schedule_active_);

        if (autoplay_action_->isChecked() && schedule_active_)
        {
            const auto method = static_cast<input_manager::idle_move>(get_weighted_int(engine_,
                *settings_->get_number_list("idle-move-probabilities")));

            input_manager_->move_random(method);

            // do this last to make sure everything else before this succeeded
            if (const auto p = settings_->get_number("refresh-regs-key"))
                input_manager_->send_keypress(static_cast<short>(*p));
        }
    }
    catch (const std::exception& e)
    {
        handle_error(e);
    }

    update_statusbar();
}

void main_window::open_strategy()
{
    const auto filename = QFileDialog::getOpenFileName(this, "Open", QString(), "Site setting files (*.xml)");

    if (filename.isEmpty())
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    load_settings(filename.toStdString());

    update_statusbar();
    
    state_widget_->set_strategy(nullptr);

    QApplication::restoreOverrideCursor();
}

void main_window::show_strategy_changed()
{
    strategy_widget_->setVisible(!strategy_widget_->isVisible());
}

void main_window::autoplay_changed(const bool checked)
{
    if (checked)
        BOOST_LOG_TRIVIAL(info) << "Enabling autoplay";
    else
        BOOST_LOG_TRIVIAL(info) << "Disabling autoplay";
}

#pragma warning(push)
#pragma warning(disable: 4512)

void main_window::process_snapshot(const int slot, const fake_window& window)
{
    if (save_images_->isChecked())
        save_snapshot();

    table_manager table(*settings_, *input_manager_, window);

    // handle sit-out as the absolute first step and react accordingly
    if (autoplay_action_->isChecked() && table.is_sitting_out())
    {
        if (settings_->get_number("auto-sit-in", 0))
        {
            BOOST_LOG_TRIVIAL(info) << "Sitting in";
            table.sit_in(settings_->get_interval("action-delay", site_settings::interval_t()).second);
        }

        throw std::runtime_error("We are sitting out");
    }

    const auto tournament_id = slot;

    BOOST_LOG_SCOPED_THREAD_TAG("TournamentID", tournament_id);

    ENSURE(tournament_id != -1);

    auto& table_data = table_data_[tournament_id];
    table_data.slot = tournament_id;

    const auto now = QDateTime::currentDateTime();

    // wait until action delay is over
    if (table_data.capture_state != table_data_t::IDLE && table_data.capture_state != table_data_t::SNAPSHOT)
        return;

    // ignore out-of-turn snapshots
    if (autoplay_action_->isChecked() && !table.is_waiting())
        return;

    const auto& snapshot = table.get_snapshot();
    const auto& prev_snapshot = get_snapshot(table_data.window);

    table_update_time_ = now;

    holdem_state::game_round round = holdem_state::INVALID_ROUND;

    if (snapshot.board[0] != -1 && snapshot.board[1] != -1 && snapshot.board[2] != -1)
    {
        if (snapshot.board[3] != -1)
        {
            if (snapshot.board[4] != -1)
                round = holdem_state::RIVER;
            else
                round = holdem_state::TURN;
        }
        else
        {
            round = holdem_state::FLOP;
        }
    }
    else if (snapshot.board[0] == -1 && snapshot.board[1] == -1 && snapshot.board[2] == -1)
    {
        round = holdem_state::PREFLOP;
    }
    else
    {
        round = holdem_state::INVALID_ROUND;
    }

    // do not manipulate state when autoplay is off
    if (!autoplay_action_->isChecked())
        return;

    if (snapshot.captcha)
    {
        if (captcha_manager_->upload_image(window.get_window_image()))
            send_email("solution required");

        captcha_manager_->query_solution();

        const auto solution = captcha_manager_->get_solution();

        if (!solution.empty())
        {
            table.input_captcha(solution + '\x0d');
            captcha_manager_->reset();
        }
    }

    // wait until our box is highlighted
    if (snapshot.highlight[0] == false)
        return;

    // wait until we see buttons
    if (snapshot.buttons == 0)
        return;

    // wait for visible cards
    if (snapshot.hole[0] == -1 || snapshot.hole[1] == -1)
        return;

    // wait until we see stack sizes (some sites hide stack size when player is sitting out)
    if (snapshot.stack == -1)
        return;

    if (table_data.capture_state == table_data_t::IDLE)
    {
        transition_state(table_data, table_data_t::IDLE, table_data_t::PRE_SNAPSHOT);

        const auto pre_snapshot_wait = settings_->get_number("pre-snapshot-wait", 1.0);

        BOOST_LOG_TRIVIAL(info) << QString("Waiting %1 seconds pre-snapshot").arg(pre_snapshot_wait).toStdString();

        QTimer::singleShot(static_cast<int>(pre_snapshot_wait * 1000), this, [this, &table_data]() {
            transition_state(table_data, table_data_t::PRE_SNAPSHOT, table_data_t::SNAPSHOT);
        });

        return;
    }

    // this will most likely fail if we can't read the cards
    ENSURE(round >= holdem_state::PREFLOP && round <= holdem_state::RIVER);

    // make sure we read everything fine
    ENSURE(snapshot.total_pot >= 0);
    ENSURE(snapshot.bet[0] >= 0);
    ENSURE(snapshot.bet[1] >= 0);
    ENSURE(snapshot.dealer[0] || snapshot.dealer[1]);

    BOOST_LOG_TRIVIAL(info) << "*** SNAPSHOT ***";
    BOOST_LOG_TRIVIAL(info) << "Snapshot:";

    for (const auto& str : QString(table_manager::snapshot_t::to_string(snapshot).c_str()).trimmed().split('\n'))
        BOOST_LOG_TRIVIAL(info) << "- " << str.toStdString();

    // check if our previous action failed for some reason (buggy clients leave buttons depressed)
    if (snapshot.hole == prev_snapshot.hole
        && snapshot.board == prev_snapshot.board
        && snapshot.stack == prev_snapshot.stack
        && snapshot.bet == prev_snapshot.bet)
    {
        BOOST_LOG_TRIVIAL(warning) << "Identical snapshots; previous action not fulfilled; reverting state";
        table_data = old_table_data_[tournament_id];
    }

    old_table_data_[tournament_id] = table_data;

    const auto new_game = is_new_game(table_data, snapshot);

    // new games should always start with sb and bb bets
    ENSURE(!new_game || (snapshot.bet[0] > 0 && snapshot.bet[1] > 0));

    // figure out the dealer (sometimes buggy clients display two dealer buttons)
    const auto dealer = (snapshot.dealer[0] && snapshot.dealer[1])
        ? (new_game ? static_cast<int>(*settings_->get_number("default-dealer")) : table_data.dealer)
        : (snapshot.dealer[0] ? 0 : (snapshot.dealer[1] ? 1 : -1));

    if (snapshot.dealer[0] && snapshot.dealer[1])
        BOOST_LOG_TRIVIAL(warning) << "Multiple dealers";
    
    BOOST_LOG_TRIVIAL(info) << "Dealer: " << dealer;

    ENSURE(dealer == 0 || dealer == 1);
    ENSURE(new_game || table_data.dealer == dealer);

    // figure out the big blind (we can't trust the window title due to buggy clients)
    const auto big_blind = get_big_blind(table_data, snapshot, new_game, dealer);

    BOOST_LOG_TRIVIAL(info) << "BB: " << big_blind;

    ENSURE(big_blind > 0);

    // calculate effective stack size
    const auto stack_size = get_effective_stack(snapshot, big_blind);

    BOOST_LOG_TRIVIAL(info) << "Stack: " << stack_size << " SB";

    ENSURE(!strategies_.empty());

    const auto it = find_nearest(strategies_, stack_size);
    const auto& strategy = *it->second;
    auto current_state = table_data.state;

    if (!new_game && table_data.stack_size != stack_size)
    {
        ENSURE(table_data.state != nullptr);

        BOOST_LOG_TRIVIAL(warning) << QString("Stack size changed during game (%1 -> %2)").arg(table_data.stack_size)
            .arg(stack_size).toStdString();

        std::vector<nlhe_state::holdem_action> actions;

        for (auto state = table_data.state; state; state = state->get_parent())
            actions.push_back(state->get_action());

        std::reverse(actions.begin(), actions.end());

        current_state = &strategy.get_root_state();

        for (const auto action : actions)
        {
            for (int i = action; i >= 0 && i != nlhe_state::FOLD; --i) // assume action sizes are ascending
            {
                if (const auto child = current_state->get_action_child(nlhe_state::holdem_action(i))) // fast-forward call below handles null
                {
                    current_state = current_state->get_action_child(action);
                    break;
                }
            }
        }

        BOOST_LOG_TRIVIAL(info) << QString("State adjusted: %1 -> %2").arg(table_data.state->to_string().c_str())
            .arg(current_state->to_string().c_str()).toStdString();
    }

    BOOST_LOG_TRIVIAL(info) << QString("Strategy file: %1")
        .arg(QFileInfo(strategy.get_strategy().get_filename().c_str()).fileName()).toStdString();

    if (new_game)
        current_state = &strategy.get_root_state();

    ENSURE(current_state != nullptr);
    ENSURE(!current_state->is_terminal());

    // if our previous all-in bet didn't succeed due to client bugs, assume we bet the minimum instead
    if (current_state->get_parent() && current_state->get_action() == nlhe_state::RAISE_A)
    {
        BOOST_LOG_TRIVIAL(warning) << "Readjusting state after unsuccessful all-in bet";

        const nlhe_state* child = nullptr;

        for (int i = 0; i < current_state->get_parent()->get_child_count(); ++i)
        {
            const auto p = current_state->get_parent()->get_child(i);

            if (p && p->get_action() == nlhe_state::CALL)
            {
                child = current_state->get_parent()->get_child(i + 1); // minimum bet

                if (!child || child->get_action() == nlhe_state::RAISE_A)
                {
                    BOOST_LOG_TRIVIAL(warning) << "Falling back to treating our failed all-in bet as a call";
                    child = p;
                }
                break;
            }
        }

        current_state = child;
        ENSURE(current_state != nullptr);
    }

    ENSURE(round >= current_state->get_round());

    // a new round has started but we did not end the previous one, opponent has called
    if (round > current_state->get_round())
    {
        BOOST_LOG_TRIVIAL(info) << "Round changed; opponent called";
        current_state = current_state->call();
    }

    while (current_state && current_state->get_round() < round)
    {
        BOOST_LOG_TRIVIAL(warning) << QString("Round mismatch (%1 < %2); assuming missing action was call")
            .arg(current_state->get_round()).arg(round).toStdString();
        current_state = current_state->call();
    }

    ENSURE(current_state != nullptr);

    // note: we modify the current state normally as this will be interpreted as a check
    if (snapshot.sit_out[1])
        BOOST_LOG_TRIVIAL(info) << "Opponent is sitting out";

    if (snapshot.all_in[1])
        BOOST_LOG_TRIVIAL(info) << "Opponent is all-in";

    if ((current_state->get_round() == holdem_state::PREFLOP && (snapshot.bet[1] > big_blind))
        || (current_state->get_round() > holdem_state::PREFLOP && (snapshot.bet[1] > 0)))
    {
        // make sure opponent allin is always terminal on his part and doesnt get translated to something else
        const double fraction = snapshot.all_in[1]
            ? nlhe_state::get_raise_factor(nlhe_state::RAISE_A)
            : (snapshot.bet[1] - snapshot.bet[0]) / (snapshot.total_pot - (snapshot.bet[1] - snapshot.bet[0]));

        ENSURE(fraction > 0);
        BOOST_LOG_TRIVIAL(info) << QString("Opponent raised to %2 (%1x pot)").arg(fraction).arg(snapshot.bet[1])
            .toStdString();
        current_state = current_state->raise(fraction); // there is an outstanding bet/raise
    }
    else if (current_state->get_round() == holdem_state::PREFLOP && dealer == 1 && snapshot.bet[1] <= big_blind)
    {
        BOOST_LOG_TRIVIAL(info) << "Facing big blind sized bet out of position preflop; opponent called";
        current_state = current_state->call();
    }
    else if (current_state->get_round() > holdem_state::PREFLOP && dealer == 0 && snapshot.bet[1] == 0)
    {
        // we are in position facing 0 sized bet, opponent has checked
        // we are oop facing big blind sized bet preflop, opponent has called
        BOOST_LOG_TRIVIAL(info) << "Facing 0-sized bet in position postflop; opponent checked";
        current_state = current_state->call();
    }

    ENSURE(current_state != nullptr);

    std::stringstream ss;
    ss << *current_state;
    BOOST_LOG_TRIVIAL(info) << QString("State: %1").arg(ss.str().c_str()).toStdString();

    // ensure it is our turn
    ENSURE((dealer == 0 && current_state->get_player() == 0)
        || (dealer == 1 && current_state->get_player() == 1));

    // ensure rounds match
    ENSURE(current_state->get_round() == round);

    // we should never reach terminal states when we have a pending action
    ENSURE(current_state->get_id() != -1);

    update_strategy_widget(*current_state, strategy, snapshot.hole, snapshot.board);

    const auto next_action = get_next_action(*current_state, strategy, snapshot);

    const auto to_call = snapshot.bet[1] - snapshot.bet[0];
    const auto invested = (snapshot.total_pot + to_call) / 2.0;
    const auto delay_factor = boost::algorithm::clamp((invested / big_blind * 2.0) / stack_size, 0.0, 1.0);

    const auto delay = settings_->get_interval("action-delay", site_settings::interval_t(0, 1));
    const auto delay_window = settings_->get_number("action-delay-window", 0.5);
    const auto min_delay = delay.first + delay_factor * (delay.second - delay.first) * delay_window;
    const auto max_delay = delay.second - (1 - delay_factor) * (delay.second - delay.first) * delay_window;
    const auto wait = get_normal_random(engine_, min_delay, max_delay);

    transition_state(table_data, table_data_t::SNAPSHOT, table_data_t::ACTION);

    BOOST_LOG_TRIVIAL(info) << QString("Waiting for %1 [%2, %3] seconds before acting").arg(wait).arg(min_delay)
        .arg(max_delay).toStdString();

    QTimer::singleShot(static_cast<int>(wait * 1000), this, std::bind(&main_window::do_action, this,
        std::ref(table_data), next_action));

    table_data.window = window;
    table_data.dealer = dealer;
    table_data.big_blind = big_blind;
    table_data.stack_size = stack_size;
    table_data.state = current_state;
}

#pragma warning(pop)

nlhe_state::holdem_action main_window::get_next_action(const nlhe_state& state, const nlhe_strategy& strategy,
    const table_manager::snapshot_t& snapshot)
{
    const int c0 = snapshot.hole[0];
    const int c1 = snapshot.hole[1];
    const int b0 = snapshot.board[0];
    const int b1 = snapshot.board[1];
    const int b2 = snapshot.board[2];
    const int b3 = snapshot.board[3];
    const int b4 = snapshot.board[4];
    int bucket = -1;
    const auto& abstraction = strategy.get_abstraction();
    auto current_state = &state;

    ENSURE(current_state != nullptr);
    ENSURE(!current_state->is_terminal());

    if (c0 != -1 && c1 != -1)
    {
        holdem_abstraction_base::bucket_type buckets;
        abstraction.get_buckets(c0, c1, b0, b1, b2, b3, b4, &buckets);
        bucket = buckets[current_state->get_round()];
    }

    ENSURE(bucket != -1);

    const int index = (snapshot.sit_out[1] && current_state->get_child(nlhe_state::CALL + 1))
        ? nlhe_state::CALL + 1
        : strategy.get_strategy().get_random_child(*current_state, bucket);
    const auto action = current_state->get_child(index)->get_action();
    const QString action_name = current_state->get_action_name(action).c_str();
    const double probability = strategy.get_strategy().get_probability(*current_state, index, bucket);

    // display non-translated strategy
    BOOST_LOG_TRIVIAL(info) << QString("Strategy: %1 (%2)").arg(action_name).arg(probability).toStdString();

    return action;
}

const nlhe_state* main_window::perform_action(const nlhe_state::holdem_action action, const nlhe_state& state,
    const fake_window& window, const double big_blind)
{
    table_manager table(*settings_, *input_manager_, window);
    const auto& snapshot = table.get_snapshot();

    int next_action;
    double raise_fraction = -1;
    std::string new_action_name = nlhe_state::get_action_name(action);
    auto current_state = &state;

    switch (action)
    {
    case nlhe_state::FOLD:
        next_action = table_manager::FOLD;
        break;
    case nlhe_state::CALL:
    {
        // ensure the hand really terminates if our abstraction says so and we would just call
        if (current_state->get_round() < holdem_state::RIVER && current_state->call()->is_terminal())
        {
            BOOST_LOG_TRIVIAL(info) << "Translating pre-river call to all-in to ensure hand terminates";
            next_action = table_manager::RAISE;
            raise_fraction = current_state->get_raise_factor(nlhe_state::RAISE_A);
            new_action_name = current_state->get_action_name(nlhe_state::RAISE_A);
        }
        else
            next_action = table_manager::CALL;
    }
    break;
    default:
        next_action = table_manager::RAISE;
        raise_fraction = current_state->get_raise_factor(action);
        break;
    }

    const auto max_action_wait = settings_->get_number("max-action-wait", 10.0);

    switch (next_action)
    {
    case table_manager::FOLD:
        BOOST_LOG_TRIVIAL(info) << "Folding";
        table.fold(max_action_wait);
        break;
    case table_manager::CALL:
        BOOST_LOG_TRIVIAL(info) << "Calling";
        table.call(max_action_wait);
        break;
    case table_manager::RAISE:
        {
            // we have to call opponent bet minus our bet
            const auto to_call = snapshot.bet[1] - snapshot.bet[0];

            // maximum bet is our remaining stack plus our bet (total raise to maxbet)
            const auto maxbet = snapshot.stack + snapshot.bet[0];

            ENSURE(maxbet > 0);

            // minimum bet is opponent bet plus the amount we have to call (or big blind whichever is larger)
            // restrict minbet to always be less than or equal to maxbet (we can't bet more than stack)
            const auto minbet = std::min(snapshot.bet[1] + std::max(big_blind, to_call), maxbet);

            ENSURE(minbet <= maxbet);

            // bet amount is opponent bet (our bet + call) plus x times the pot after our call (total_pot + to_call)
            auto amount = snapshot.bet[1] + raise_fraction * (snapshot.total_pot + to_call);

            if (const auto p = settings_->get_number("bet-rounding"))
            {
                const auto rounded = round_multiple(amount, *p);

                if (rounded != amount)
                {
                    BOOST_LOG_TRIVIAL(info) << QString("Bet rounded (%1 -> %2, multiple %3)")
                        .arg(amount).arg(rounded).arg(*p).toStdString();
                    amount = rounded;
                }
            }

            amount = boost::algorithm::clamp(amount, minbet, maxbet);

            // translate bets close to our or opponents remaining stack sizes to all-in
            if (new_action_name != current_state->get_action_name(nlhe_state::RAISE_A))
            {
                const auto opp_maxbet =
                    *settings_->get_number("total-chips") - (snapshot.stack + snapshot.total_pot - snapshot.bet[1]);

                ENSURE(opp_maxbet > 0);

                const auto allin_fraction = amount / maxbet;
                const auto opp_allin_fraction = amount / opp_maxbet;
                const auto allin_threshold = settings_->get_number("allin-threshold");

                if (allin_threshold && allin_fraction >= *allin_threshold)
                {
                    BOOST_LOG_TRIVIAL(info) << QString(
                        "Bet (%1) close to our remaining stack (%2); translating to all-in (%3 >= %4)").
                        arg(amount).arg(maxbet).arg(allin_fraction).arg(*allin_threshold).toStdString();
                    amount = maxbet;
                    new_action_name = current_state->get_action_name(nlhe_state::RAISE_A);
                }
                else if (allin_threshold && opp_allin_fraction >= *allin_threshold)
                {
                    BOOST_LOG_TRIVIAL(info) << QString(
                        "Bet (%1) close to opponent remaining stack (%2); translating to all-in (%3 >= %4)").
                        arg(amount).arg(opp_maxbet).arg(opp_allin_fraction).arg(*allin_threshold).toStdString();
                    amount = maxbet;
                    new_action_name = current_state->get_action_name(nlhe_state::RAISE_A);
                }
            }

            const auto method = static_cast<table_manager::raise_method>(get_weighted_int(engine_,
                *settings_->get_number_list("bet-method-probabilities")));

            BOOST_LOG_TRIVIAL(info) << QString("Raising to %1 [%3, %5] (%2x pot) (method %4)").arg(amount)
                .arg(raise_fraction).arg(minbet).arg(method).arg(maxbet).toStdString();

            table.raise(new_action_name, amount, minbet, max_action_wait, method);
        }
        break;
    }

    // update state pointer last after the table_manager input functions
    // this makes it possible to recover in case the buttons are stuck but become normal again as the state pointer
    // won't be in an invalid (terminal) state
    current_state = current_state->get_action_child(action);

    ENSURE(current_state != nullptr);

    return current_state;
}

void main_window::handle_schedule()
{
    if (!schedule_action_->isChecked())
        return;

    const auto schedule_path = settings_->get_string("schedule");
    const auto spans = schedule_path ? read_schedule_file(*schedule_path) : spans_t();
    const auto active = is_schedule_active(spans, &next_activity_date_);

    if (active != schedule_active_)
    {
        schedule_active_ = active;

        if (schedule_active_)
            BOOST_LOG_TRIVIAL(info) << "Enabling scheduled registration";
        else
            BOOST_LOG_TRIVIAL(info) << "Disabling scheduled registration";

        BOOST_LOG_TRIVIAL(info) << make_schedule_string(schedule_active_, next_activity_date_).toStdString();
    }
}

void main_window::modify_state_changed()
{
    state_widget_->setVisible(!state_widget_->isVisible());
}

void main_window::state_widget_board_changed(const QString& board)
{
    std::string s(QString(board).remove(' ').toStdString());
    std::array<int, 5> b;
    b.fill(-1);

    for (auto i = 0; i < b.size(); ++i)
    {
        if (s.size() < 2)
            break;

        b[i] = string_to_card(s);
        s.erase(s.begin(), s.begin() + 2);
    }

    strategy_widget_->set_board(b);

    if (state_widget_->get_strategy() && state_widget_->get_state())
        strategy_widget_->update(*state_widget_->get_strategy(), *state_widget_->get_state());
}

void main_window::update_strategy_widget(const nlhe_state& state, const nlhe_strategy& strategy,
    const std::array<int, 2>& hole, const std::array<int, 5>& board)
{
    strategy_widget_->set_hole(hole);
    strategy_widget_->set_board(board);
    strategy_widget_->update(strategy, state);
}

void main_window::ensure(bool expression, const std::string& s, int line) const
{
    if (expression)
        return;

    assert(false);
    BOOST_LOG_TRIVIAL(error) << QString("Verification on line %1 failed (%2)").arg(line).arg(s.c_str()).toStdString();

    throw std::runtime_error(s.c_str());
}

void main_window::schedule_changed(const bool checked)
{
    if (checked)
        BOOST_LOG_TRIVIAL(info) << "Enabling scheduler";
    else
    {
        BOOST_LOG_TRIVIAL(info) << "Disabling scheduler";
        schedule_active_ = false;
    }
}

void main_window::state_widget_state_changed()
{
    if (!state_widget_->get_strategy() || !state_widget_->get_state())
        return;

    strategy_widget_->update(*state_widget_->get_strategy(), *state_widget_->get_state());
}

void main_window::update_statusbar()
{
    site_label_->setText(settings_->get_filename().c_str());
    strategy_label_->setText(QString("%1 strategies").arg(strategies_.size()));

    QString s;

    if (schedule_action_->isChecked())
        s = make_schedule_string(schedule_active_, next_activity_date_);
    else
        s = "No schedule";

    schedule_label_->setText(s);
}

void main_window::load_settings(const std::string& filename)
{
    if (filename.empty())
        return;

    sitename_ = QFileInfo(filename.c_str()).baseName().toStdString();

    settings_->load(filename);

    BOOST_LOG_TRIVIAL(info) << "gui " << util::GIT_VERSION;

    const auto input_delay = settings_->get_interval("input-delay", site_settings::interval_t(0, 1000));
    const auto double_click_delay = settings_->get_interval("double-click-delay", site_settings::interval_t(0, 1000));
    const auto mouse_speed = settings_->get_interval("mouse-speed", site_settings::interval_t(0, 1000));

    input_manager_->set_delay(input_delay.first, input_delay.second);
    input_manager_->set_double_click_delay(double_click_delay.first, double_click_delay.second);
    input_manager_->set_mouse_speed(mouse_speed.first, mouse_speed.second);

    title_filter_->setText(QString::fromStdString(settings_->get_string("title-filter", "")));

    capture_timer_->setInterval(static_cast<int>(settings_->get_number("capture", 1.0) * 1000.0));

    if (const auto smtp_host = settings_->get_string("smtp-host"))
    {
        smtp_ = new smtp(smtp_host->c_str(), static_cast<std::uint16_t>(*settings_->get_number("smtp-port")));

        connect(smtp_, &smtp::status, [](const QString& s)
        {
            BOOST_LOG_TRIVIAL(info) << s.toStdString();
        });
    }
    else
        smtp_ = nullptr;

    if (const auto p = settings_->get_string("captcha-read-url"))
        captcha_manager_->set_read_url(*p);

    if (const auto p = settings_->get_string("captcha-upload-url"))
        captcha_manager_->set_upload_url(*p);

    table_windows_.clear();

    for (const auto& w : settings_->get_windows("table"))
        table_windows_.push_back(std::unique_ptr<fake_window>(new fake_window(*w.second, *settings_, *window_manager_)));

    strategies_.clear();

    for (const auto& p : settings_->get_strings("strategy"))
    {
        const auto& filename = *p.second;

        try
        {
            std::unique_ptr<nlhe_strategy> p(new nlhe_strategy(filename));
            strategies_[p->get_stack_size()] = std::move(p);
        }
        catch (const std::exception& e)
        {
            BOOST_LOG_TRIVIAL(fatal) << e.what();
            continue;
        }

        BOOST_LOG_TRIVIAL(info) << QString("Loaded strategy: %1").arg(filename.c_str()).toStdString();
    }

    BOOST_LOG_TRIVIAL(info) << strategies_.size() << " strategies loaded";

    max_error_count_ = settings_->get_number("max-error-count", 5.0);
    error_allowance_ = max_error_count_;

    BOOST_LOG_TRIVIAL(info) << QString("Loaded site settings: %1").arg(filename.c_str()).toStdString();
}

int main_window::get_effective_stack(const table_manager::snapshot_t& snapshot, const double big_blind) const
{
    // figure out the effective stack size

    // our stack size should always be visible
    ENSURE(snapshot.stack > 0);

    std::array<double, 2> stacks;

    stacks[0] = snapshot.stack + (snapshot.total_pot - snapshot.bet[0] - snapshot.bet[1]) / 2.0 + snapshot.bet[0];
    stacks[1] = *settings_->get_number("total-chips") - stacks[0];

    // both stacks sizes should be known by now
    ENSURE(stacks[0] > 0 && stacks[1] > 0);

    const auto stack_size = static_cast<int>(std::ceil(std::min(stacks[0], stacks[1]) / big_blind * 2.0));

    ENSURE(stack_size > 0);

    return stack_size;
}

bool main_window::is_new_game(const table_data_t& table_data, const table_manager::snapshot_t& snapshot) const
{
    const auto& prev_snapshot = get_snapshot(table_data.window);

    // new games always start with zero board cards
    if (snapshot.board[0] != -1
        || snapshot.board[1] != -1
        || snapshot.board[2] != -1
        || snapshot.board[3] != -1
        || snapshot.board[4] != -1)
    {
        return false;
    }

    // dealer button is not bugged and it has changed between snapshots -> new game
    // (dealer button status can change mid game on buggy clients)
    if (snapshot.dealer[0] != snapshot.dealer[1] &&
        prev_snapshot.dealer[0] != prev_snapshot.dealer[1] &&
        snapshot.dealer != prev_snapshot.dealer)
    {
        BOOST_LOG_TRIVIAL(info) << "New game (dealer changed)";
        return true;
    }

    // hole cards changed -> new game
    if (snapshot.hole != prev_snapshot.hole)
    {
        BOOST_LOG_TRIVIAL(info) << "New game (hole cards changed)";
        return true;
    }

    // stack size can never increase during a game between snapshots, so a new game must have started
    // only check if stack data is valid (not sitting out or all in)
    if (snapshot.stack > 0 && prev_snapshot.stack > 0 && snapshot.stack > prev_snapshot.stack)
    {
        BOOST_LOG_TRIVIAL(info) << "New game (stack increased)";
        return true;
    }

    // we know the previous hand ended due to our terminal action (fold, call allin or river, raise allin)
    if (table_data.state && (table_data.state->is_terminal() || table_data.state->get_action() == nlhe_state::RAISE_A))
    {
        BOOST_LOG_TRIVIAL(info) << "New game (previous state was terminal)";
        return true;
    }

    // opponent was all-in but isn't anymore -> should be handled by check above as all our actions are terminal
    ENSURE(!(!snapshot.all_in[1] && prev_snapshot.all_in[1]));

    // TODO: a false negatives can be possible in some corner cases
    return false;
}

void main_window::save_snapshot() const
{
    const auto p = settings_->get_number("save-snapshots");

    if (!(p && *p))
        return;

    BOOST_LOG_TRIVIAL(info) << "Saving current snapshot";

    const auto& image = window_manager_->get_image();
    image.save(QDateTime::currentDateTimeUtc().toString("'snapshot-'yyyy-MM-ddTHHmmss.zzz'.png'"));
}

void main_window::update_capture()
{
    QTime t;
    t.start();

    while (t.elapsed() <= static_cast<int>(settings_->get_number("tooltip-move-time", 0.0) * 1000))
    {
        if (try_capture())
            return;
        else
        {
            BOOST_LOG_TRIVIAL(info) << "Retrying capture";

            if (autoplay_action_->isChecked())
            {
                input_manager_->move_random(input_manager::IDLE_MOVE_DESKTOP);
                input_manager_->sleep();
            }
        }
    }

    throw std::runtime_error("Unable to get valid capture");
}

bool main_window::try_capture()
{
    window_manager_->update(title_filter_->text().toStdString());

    for (auto& i : table_windows_)
    {
        if (!i->update())
            return false;
    }

    for (const auto& button : settings_->get_buttons("bad"))
    {
        for (const auto& table : table_windows_)
        {
            const auto& rect = button.second->unscaled_rect;

            if (table->is_mouse_inside(*input_manager_, rect))
            {
                const auto bad_rect = table->get_scaled_rect(rect);

                BOOST_LOG_TRIVIAL(warning) << QString("Mouse in bad position (%1,%2,%3,%4)")
                    .arg(bad_rect.x())
                    .arg(bad_rect.y())
                    .arg(bad_rect.width())
                    .arg(bad_rect.height()).toStdString();

                return false;
            }
        }
    }

    const auto tooltip_rect = window_manager_->get_tooltip();

    if (tooltip_rect.isValid())
    {
        BOOST_LOG_TRIVIAL(warning) << QString("Tooltip found (%1,%2,%3,%4)")
            .arg(tooltip_rect.x())
            .arg(tooltip_rect.y())
            .arg(tooltip_rect.width())
            .arg(tooltip_rect.height()).toStdString();

        return false;
    }

    return true;
}

void main_window::send_email(const std::string& subject, const std::string& message)
{
    if (!smtp_ || !settings_)
        return;

    smtp_->send(settings_->get_string("smtp-from")->c_str(), settings_->get_string("smtp-to")->c_str(),
        QString("[midas] [%1] %2").arg(sitename_.c_str()).arg(subject.c_str()), message.c_str());
}

void main_window::check_idle(const bool schedule_active)
{
    if (schedule_active)
    {
        const auto now = QDateTime::currentDateTime();

        if (table_update_time_.isNull())
            table_update_time_ = now;

        const auto max_idle_time = settings_->get_number("max-idle-time");

        if (max_idle_time && table_update_time_.msecsTo(now) > *max_idle_time * 1000.0)
        {
            BOOST_LOG_TRIVIAL(warning) << "We are idle";
            table_update_time_ = QDateTime();
            send_email("idle");
            save_snapshot();
        }
    }
    else
        table_update_time_ = QDateTime();
}

void main_window::handle_error(const std::exception& e)
{
    BOOST_LOG_TRIVIAL(error) << e.what();

    save_snapshot();

    if (!schedule_action_->isChecked())
        return;

    const auto max_error_interval = settings_->get_number("max-error-interval", 60.0);

    const auto now = QDateTime::currentDateTime();
    const auto elapsed = error_time_.msecsTo(now) / 1000.0;
    error_time_ = now;

    error_allowance_ += elapsed * (max_error_count_ / max_error_interval);

    if (error_allowance_ > max_error_count_)
        error_allowance_ = max_error_count_;

    if (error_allowance_ < 1.0)
    {
        schedule_action_->setChecked(false);
        send_email("fatal error");
        BOOST_LOG_TRIVIAL(fatal) << "Maximum errors reached; stopping registrations";
    }
    else
    {
        error_allowance_ -= 1.0;

        BOOST_LOG_TRIVIAL(warning) << QString("Error allowance: %1/%2").arg(error_allowance_)
            .arg(max_error_count_).toStdString();
    }
}

double main_window::get_big_blind(const table_data_t& table_data, const table_manager::snapshot_t& snapshot,
    const bool new_game, const int dealer) const
{
    const auto big_blind = new_game
        ? (dealer == 0 ? 2.0 : 1.0) * snapshot.bet[0]
        : table_data.big_blind;

    if (big_blind <= 0)
    {
        const auto default_blind = settings_->get_number("default-big-blind", 20);

        BOOST_LOG_TRIVIAL(warning) << QString("Invalid blind; using default (%1)").arg(default_blind).toStdString();

        return default_blind;
    }
    else
        return big_blind;
}

table_manager::snapshot_t main_window::get_snapshot(const fake_window& window) const
{
    return window.is_valid()
        ? table_manager(*settings_, *input_manager_, window).get_snapshot()
        : table_manager::snapshot_t();
}

void main_window::do_action(table_data_t& table_data, const nlhe_state::holdem_action action)
{
    try
    {
        BOOST_LOG_SCOPED_THREAD_TAG("TournamentID", table_data.slot);

        ENSURE(table_data.state != nullptr);

        table_data.state = perform_action(action, *table_data.state, table_data.window, table_data.big_blind);

        const auto post_action_wait = settings_->get_number("post-action-wait", 5.0);

        transition_state(table_data, table_data_t::ACTION, table_data_t::POST_ACTION);

        BOOST_LOG_TRIVIAL(info) << QString("Waiting %1 seconds post-action").arg(post_action_wait).toStdString();

        QTimer::singleShot(static_cast<int>(post_action_wait * 1000), this, [this, &table_data]() {
            transition_state(table_data, table_data_t::POST_ACTION, table_data_t::IDLE);
        });
    }
    catch (const std::exception& e)
    {
        reset_state(table_data);
        handle_error(e);
    }
}

void main_window::transition_state(table_data_t& table_data, const table_data_t::state_t old,
    const table_data_t::state_t neu)
{
    try
    {
        BOOST_LOG_SCOPED_THREAD_TAG("TournamentID", table_data.slot);
        BOOST_LOG_TRIVIAL(info) << QString("Capture state transition: %1 -> %2")
            .arg(table_data_t::to_string(old).c_str()).arg(table_data_t::to_string(neu).c_str()).toStdString();

        ENSURE(table_data.capture_state == old);
        table_data.capture_state = neu;
    }
    catch (const std::exception& e)
    {
        handle_error(e);
    }
}

std::string main_window::table_data_t::to_string(const state_t& state)
{
    switch (state)
    {
    case main_window::table_data_t::IDLE: return "IDLE";
    case main_window::table_data_t::PRE_SNAPSHOT: return "PRE_SNAPSHOT";
    case main_window::table_data_t::SNAPSHOT: return "SNAPSHOT";
    case main_window::table_data_t::ACTION: return "ACTION"; break;
    case main_window::table_data_t::POST_ACTION: return "POST_ACTION";
    default: return "N/A";
    }
}

void main_window::reset_state(table_data_t& table_data)
{
    const auto& old = table_data.capture_state;
    const auto& neu = table_data_t::IDLE;

    BOOST_LOG_SCOPED_THREAD_TAG("TournamentID", table_data.slot);
    BOOST_LOG_TRIVIAL(info) << QString("Resetting state: %1 -> %2")
        .arg(table_data_t::to_string(old).c_str()).arg(table_data_t::to_string(neu).c_str()).toStdString();

    table_data.capture_state = neu;
}
