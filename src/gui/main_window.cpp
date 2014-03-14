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
#include "table_widget.h"
#include "holdem_strategy_widget.h"
#include "table_manager.h"
#include "input_manager.h"
#include "lobby_manager.h"
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
    template<class T>
    typename T::const_iterator find_nearest(const T& map, const typename T::key_type& value)
    {
        const auto it = map.lower_bound(value);
        return it != map.end() ? it : boost::prior(map.end());
    }

    QString secs_to_hms(double seconds_)
    {
        auto seconds = static_cast<int>(seconds_);

        const auto hours = static_cast<int>(seconds / 3600);
        seconds -= hours * 3600;

        const auto minutes = static_cast<int>(seconds / 60);
        seconds -= minutes * 60;

        return QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    }

    bool is_schedule_active(const double var, const double day_variance, const site_settings::spans_t& daily_spans,
        double* time)
    {
        assert(daily_spans.size() == 7);

        const auto t = QDateTime::currentDateTime();
        const auto day = t.date().dayOfWeek() - 1;
        const auto& spans = daily_spans[day];
        const auto now = window_utils::datetime_to_secs(t);

        std::mt19937_64 engine(t.date().toJulianDay());

        const auto offset = get_uniform_random(engine, -day_variance, day_variance);

        for (const auto span : spans)
        {
            const auto first = get_uniform_random(engine, span.first - var + offset, span.first + var + offset);
            const auto second = get_uniform_random(engine, span.second - var + offset, span.second + var + offset);

            if (now < first)
            {
                *time = first - now;
                return false;
            }
            else if (now >= first && now < second)
            {
                *time = second - now;
                return true;
            }
        }

        int days = 1;

        for (;;)
        {
            const auto i = (day + days) % daily_spans.size();

            if (!daily_spans[i].empty())
            {
                // TODO use get_uniform_random
                *time = days * 86400.0 + daily_spans[i][0].first - now;
                return false;
            }

            if (i == day)
                break; // we wrapped around

            ++days;
        }

        *time = -1;

        return false;
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
}

main_window::main_window()
    : engine_(std::random_device()())
    , input_manager_(new input_manager)
    , schedule_active_(false)
    , time_to_next_activity_(0)
    , settings_(new site_settings)
    , smtp_(nullptr)
    , captcha_manager_(new captcha_manager)
    , window_manager_(new window_manager)
{
    auto widget = new QWidget(this);
    widget->setFocus();

    setCentralWidget(widget);

    visualizer_ = new table_widget(this);
    strategy_widget_ = new holdem_strategy_widget(this, Qt::Tool);
    strategy_widget_->setVisible(false);
    state_widget_ = new state_widget(this, Qt::Tool);
    state_widget_->setVisible(false);
    connect(state_widget_, &state_widget::board_changed, this, &main_window::state_widget_board_changed);
    connect(state_widget_, &state_widget::state_changed, this, &main_window::state_widget_state_changed);

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
    table_count_ = new QSpinBox(this);
    table_count_->setValue(0);
    table_count_->setRange(0, 100);
    toolbar->addWidget(table_count_);
    toolbar->addSeparator();

    schedule_action_ = toolbar->addAction(QIcon(":/icons/time.png"), "Schedule");
    schedule_action_->setCheckable(true);
    connect(schedule_action_, &QAction::toggled, this, &main_window::schedule_changed);

    autoplay_action_ = toolbar->addAction(QIcon(":/icons/control_play.png"), "Autoplay");
    autoplay_action_->setCheckable(true);
    connect(autoplay_action_, &QAction::toggled, this, &main_window::autoplay_changed);

    autolobby_action_ = toolbar->addAction(QIcon(":/icons/layout.png"), "Autolobby");
    autolobby_action_->setCheckable(true);
    connect(autolobby_action_, &QAction::toggled, this, &main_window::autolobby_changed);

    log_ = new QPlainTextEdit(this);
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(1000);

    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->addWidget(visualizer_);
    layout->addWidget(log_);
    widget->setLayout(layout);

    capture_timer_ = new QTimer(this);
    connect(capture_timer_, &QTimer::timeout, this, &main_window::capture_timer_timeout);

    site_label_ = new QLabel(this);
    statusBar()->addWidget(site_label_, 1);
    strategy_label_ = new QLabel(this);
    statusBar()->addWidget(strategy_label_, 1);
    schedule_label_ = new QLabel(this);
    statusBar()->addWidget(schedule_label_, 1);
    registered_label_ = new QLabel(this);
    statusBar()->addWidget(registered_label_, 1);
    update_statusbar();

    boost::log::add_common_attributes();
    boost::log::register_sink_factory("Window", boost::make_shared<qt_sink_factory>(*log_));
    boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");

    BOOST_LOG_TRIVIAL(info) << "Starting capture";

    capture_timer_->start(1000);
    mark_time_.start();

    setWindowTitle(QString("gui %1").arg(util::GIT_VERSION));
}

main_window::~main_window()
{
}

void main_window::capture_timer_timeout()
{
    try
    {
        if (const auto p = settings_->get_interval("capture"))
        {
            const auto interval = get_uniform_random(engine_, p->first, p->second);
            capture_timer_->setInterval(static_cast<int>(interval * 1000.0));
        }

        if (const auto p = settings_->get_number("mark"))
        {
            if (mark_time_.elapsed() >= static_cast<int>(*p * 1000.0))
            {
                mark_time_.restart();
                BOOST_LOG_TRIVIAL(info) << "-- MARK --";
            }
        }

        if (!autolobby_action_->isChecked() && QFileInfo("enable.txt").exists())
        {
            BOOST_LOG_TRIVIAL(info) << "enable.txt found, enabling autolobby";
            autolobby_action_->setChecked(true);
        }

        if (autolobby_action_->isChecked() && QFileInfo("disable.txt").exists())
        {
            BOOST_LOG_TRIVIAL(info) << "disable.txt found, disabling autolobby";
            autolobby_action_->setChecked(false);
        }

        if (const auto p = settings_->get_number("tooltip-move-time"))
        {
            QRect tooltip_rect;

            QTime t;
            t.start();

            while (t.elapsed() <= *p * 1000.0)
            {
                window_manager_->update(title_filter_->text().toStdString());

                tooltip_rect = window_manager_->get_tooltip();

                if (tooltip_rect.isNull())
                    break;

                BOOST_LOG_TRIVIAL(warning) << "Tooltip found; retrying capture";

                if (autoplay_action_->isChecked())
                {
                    input_manager_->move_random(input_manager::IDLE_MOVE_DESKTOP);
                    input_manager_->sleep();
                }
            }

            if (tooltip_rect.isValid())
            {
                throw std::runtime_error(QString("Tooltip found at (%1,%2,%3,%4)")
                    .arg(tooltip_rect.x())
                    .arg(tooltip_rect.y())
                    .arg(tooltip_rect.width())
                    .arg(tooltip_rect.height()).toStdString().c_str());
            }
        }

        handle_schedule();

        if (lobby_ && !window_manager_->get_image().isNull())
        {
            lobby_->update_windows();

            std::vector<const fake_window*> tables;
            
            for (const auto& window : lobby_->get_tables())
                tables.push_back(window.get());

            std::shuffle(tables.begin(), tables.end(), engine_);

            for (const auto& window : tables)
            {
                if (window->is_valid())
                    process_snapshot(*window);
            }

            remove_old_table_data();

            if (autolobby_action_->isChecked())
                handle_lobby();
        }

        if (autolobby_action_->isChecked() && (!schedule_action_->isChecked() || schedule_active_))
        {
            const auto method = static_cast<input_manager::idle_move>(get_weighted_int(engine_,
                *settings_->get_number_list("idle-move-probabilities")));

            input_manager_->move_random(method);
        }
    }
    catch (const std::exception& e)
    {
        BOOST_LOG_TRIVIAL(fatal) << e.what();

        if (site_)
        {
            BOOST_LOG_TRIVIAL(warning) << "Saving current snapshot";
            save_snapshot();
        }
    }

    update_statusbar();
}

void main_window::open_strategy()
{
    const auto filenames = QFileDialog::getOpenFileNames(this, "Open", QString(), "All files (*.*);;Site setting files (*.xml);;Strategy files (*.str)");

    if (filenames.empty())
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    std::map<int, std::unique_ptr<nlhe_strategy>> new_strategies;

    std::regex r("([^-]+)-([^-]+)-[0-9]+\\.str");
    std::regex r_nlhe("nlhe\\.([a-z]+)\\.([0-9]+)");
    std::smatch m;
    std::smatch m_nlhe;

    for (auto i = filenames.begin(); i != filenames.end(); ++i)
    {
        const auto info = QFileInfo(*i);

        if (info.suffix() == "xml")
        {
            const auto filename = i->toStdString();
            load_settings(filename);
            lobby_.reset(new lobby_manager(*settings_, *input_manager_, *window_manager_));
            site_.reset(new table_manager(*settings_, *input_manager_));

            BOOST_LOG_TRIVIAL(info) << QString("Loaded site settings: %1").arg(filename.c_str()).toStdString();
        }
        else if (info.suffix() == "str")
        {
            const std::string filename = QFileInfo(*i).fileName().toStdString();

            try
            {
                std::unique_ptr<nlhe_strategy> p(new nlhe_strategy(i->toStdString()));
                new_strategies[p->get_stack_size()] = std::move(p);
            }
            catch (const std::exception& e)
            {
                BOOST_LOG_TRIVIAL(fatal) << e.what();
                continue;
            }

            BOOST_LOG_TRIVIAL(info) << QString("Loaded strategy: %1").arg(*i).toStdString();
        }
    }

    if (!new_strategies.empty())
        std::swap(strategies_, new_strategies);

    update_statusbar();
    
    state_widget_->set_root_state(strategies_.empty() ? nullptr : &strategies_.begin()->second->get_root_state());

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

void main_window::process_snapshot(const fake_window& window)
{
    const auto tournament_id = lobby_->get_tournament_id(window);

    BOOST_LOG_SCOPED_THREAD_TAG("TournamentID", tournament_id);

    ENSURE(tournament_id != -1);

    auto& table_data = table_data_[tournament_id];
    table_data.timestamp = QDateTime::currentDateTime();

    const auto& snapshot = site_->update(window);

    if (save_images_->isChecked())
        save_snapshot();

    // consider sitout fatal
    if (snapshot.sit_out[0])
    {
        if (table_count_->value() > 0)
        {
            // this bypasses problem with schedules which could retoggle registrations if disabled
            table_count_->setValue(0);

            if (smtp_)
            {
                smtp_->send(settings_->get_string("smtp-from")->c_str(), settings_->get_string("smtp-to")->c_str(),
                    "[midas] " + QHostInfo::localHostName() + " needs attention", ".");
            }

            throw std::runtime_error("We are sitting out; stopping registrations");
        }

        return;
    }

    // these should work for observed tables
    visualizer_->clear_row(tournament_id);
    visualizer_->set_dealer(tournament_id, snapshot.dealer[0] ? 0 : (snapshot.dealer[1] ? 1 : -1));
    visualizer_->set_real_pot(tournament_id, snapshot.total_pot);
    visualizer_->set_real_bets(tournament_id, snapshot.bet[0], snapshot.bet[1]);
    visualizer_->set_sit_out(tournament_id, snapshot.sit_out[0], snapshot.sit_out[1]);
    visualizer_->set_stacks(tournament_id, snapshot.stack[0], snapshot.stack[1]);
    visualizer_->set_buttons(tournament_id, snapshot.buttons);
    visualizer_->set_all_in(tournament_id, snapshot.all_in[0], snapshot.all_in[1]);

    holdem_state::game_round round = holdem_state::INVALID_ROUND;

    visualizer_->set_hole_cards(tournament_id, snapshot.hole);
    visualizer_->set_board_cards(tournament_id, snapshot.board);

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
        {
            if (smtp_)
            {
                smtp_->send(settings_->get_string("smtp-from")->c_str(), settings_->get_string("smtp-to")->c_str(),
                    "[midas] " + QHostInfo::localHostName() + " requests solution", ".");
            }
        }

        captcha_manager_->query_solution();

        const auto solution = captcha_manager_->get_solution();

        if (!solution.empty())
        {
            site_->input_captcha(solution + '\x0d');
            captcha_manager_->reset();
        }
    }

    // wait until our box is highlighted
    if (!snapshot.highlight[0])
        return;

    // wait until we see buttons
    if (snapshot.buttons == 0)
        return;

    // wait for visible cards
    if (snapshot.hole[0] == -1 || snapshot.hole[1] == -1)
        return;

    // wait until we see stack sizes (some sites hide stack size when player is sitting out)
    if (snapshot.stack[0] == 0 || (snapshot.stack[1] == 0 && !snapshot.all_in[1] && !snapshot.sit_out[1]))
        return;

    // this will most likely fail if we can't read the cards
    ENSURE(round >= holdem_state::PREFLOP && round <= holdem_state::RIVER);

    // make sure we read everything fine
    ENSURE(snapshot.total_pot >= 0);
    ENSURE(snapshot.bet[0] >= 0);
    ENSURE(snapshot.bet[1] >= 0);
    ENSURE(snapshot.dealer[0] || snapshot.dealer[1]);

    // process action delay last after every other possible bailout
    const auto now = QDateTime::currentDateTime();
    auto& next_action_time = table_data.next_action_time;

    if (!next_action_time.isValid())
    {
        const auto& action_delay = *settings_->get_interval("action-delay");

        const double wait = std::max(0.0, (snapshot.sit_out[1] ? action_delay.first
            : get_normal_random(engine_, action_delay.first, action_delay.second)));

        BOOST_LOG_TRIVIAL(info) << QString("Waiting for %1 seconds before acting").arg(wait).toStdString();

        next_action_time = now.addMSecs(static_cast<qint64>(wait * 1000));
    }

    // wait until action delay is over
    if (now < next_action_time)
        return; // this should be the last "return" in snapshot processing

    next_action_time = QDateTime();

    // check if our previous action failed for some reason (buggy clients leave buttons depressed)
    if (snapshot.hole == table_data.snapshot.hole
        && snapshot.board == table_data.snapshot.board
        && snapshot.stack == table_data.snapshot.stack
        && snapshot.bet == table_data.snapshot.bet)
    {
        BOOST_LOG_TRIVIAL(warning) << "Identical snapshots; previous action not fulfilled; reverting state";
        table_data = old_table_data_[tournament_id];
    }

    old_table_data_[tournament_id] = table_data;

    BOOST_LOG_TRIVIAL(info) << "*** SNAPSHOT ***";
    BOOST_LOG_TRIVIAL(info) << "Window: " << window.get_window_text();

    const auto new_game = is_new_game(table_data, snapshot);

    // figure out the dealer (sometimes buggy clients display two dealer buttons)
    const auto dealer = (snapshot.dealer[0] && snapshot.dealer[1])
        ? (new_game ? 1 - table_data.dealer : table_data.dealer)
        : (snapshot.dealer[0] ? 0 : (snapshot.dealer[1] ? 1 : -1));

    if (snapshot.dealer[0] && snapshot.dealer[1])
        BOOST_LOG_TRIVIAL(warning) << "Multiple dealers";
    
    BOOST_LOG_TRIVIAL(info) << "Dealer: " << dealer;

    ENSURE(dealer == 0 || dealer == 1);
    ENSURE(new_game || table_data.dealer == dealer);

    // figure out the big blind (we can't trust the window title due to buggy clients)
    const auto big_blind = new_game
        ? (dealer == 0 ? 2.0 : 1.0) * snapshot.bet[0]
        : table_data.big_blind;

    BOOST_LOG_TRIVIAL(info) << "BB: " << big_blind;

    ENSURE(big_blind > 0);

    // calculate effective stack size
    const auto stack_size = get_effective_stack(snapshot, big_blind);

    BOOST_LOG_TRIVIAL(info) << "Stack: " << stack_size << " SB";

    ENSURE(new_game || table_data.stack_size == stack_size);

    if (snapshot.hole[0] != -1 && snapshot.hole[1] != -1)
    {
        BOOST_LOG_TRIVIAL(info) << QString("Hole: [%1 %2]").arg(get_card_string(snapshot.hole[0]).c_str())
            .arg(get_card_string(snapshot.hole[1]).c_str()).toStdString();
    }

    QString board_string;

    if (snapshot.board[4] != -1)
    {
        board_string = QString("Board: [%1 %2 %3 %4] [%5]").arg(get_card_string(snapshot.board[0]).c_str())
            .arg(get_card_string(snapshot.board[1]).c_str()).arg(get_card_string(snapshot.board[2]).c_str())
            .arg(get_card_string(snapshot.board[3]).c_str()).arg(get_card_string(snapshot.board[4]).c_str());
    }
    else if (snapshot.board[3] != -1)
    {
        board_string = QString("Board: [%1 %2 %3] [%4]").arg(get_card_string(snapshot.board[0]).c_str())
            .arg(get_card_string(snapshot.board[1]).c_str()).arg(get_card_string(snapshot.board[2]).c_str())
            .arg(get_card_string(snapshot.board[3]).c_str());
    }
    else if (snapshot.board[0] != -1)
    {
        board_string = QString("Board: [%1 %2 %3]").arg(get_card_string(snapshot.board[0]).c_str())
            .arg(get_card_string(snapshot.board[1]).c_str()).arg(get_card_string(snapshot.board[2]).c_str());
    }

    if (!board_string.isEmpty())
        BOOST_LOG_TRIVIAL(info) << board_string.toStdString();

    ENSURE(!strategies_.empty());

    auto it = find_nearest(strategies_, stack_size);
    auto& strategy = *it->second;
    auto& current_state = table_data.state;

    BOOST_LOG_TRIVIAL(info) << QString("Strategy file: %1")
        .arg(QFileInfo(strategy.get_strategy().get_filename().c_str()).fileName()).toStdString();

    if (new_game)
        current_state = &strategy.get_root_state();

    ENSURE(current_state != nullptr);
    ENSURE(!current_state->is_terminal());

    ENSURE(round >= current_state->get_round());

    // a new round has started but we did not end the previous one, opponent has called
    if (round > current_state->get_round())
    {
        BOOST_LOG_TRIVIAL(info) << "Round changed; opponent called";
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
        BOOST_LOG_TRIVIAL(info) << QString("Opponent raised %1x pot").arg(fraction).toStdString();
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

    update_strategy_widget(tournament_id, strategy, snapshot.hole, snapshot.board);

    perform_action(tournament_id, strategy, snapshot, big_blind);

    table_data.snapshot = snapshot;
    table_data.dealer = dealer;
    table_data.big_blind = big_blind;
    table_data.stack_size = stack_size;

    // update snapshot timestamp again to ignore any input duration in old table calculation
    table_data.timestamp = QDateTime::currentDateTime();
}

#pragma warning(pop)

void main_window::perform_action(const tid_t tournament_id, const nlhe_strategy& strategy,
    const table_manager::snapshot_t& snapshot, const double big_blind)
{
    ENSURE(site_ && !strategies_.empty());

    const int c0 = snapshot.hole[0];
    const int c1 = snapshot.hole[1];
    const int b0 = snapshot.board[0];
    const int b1 = snapshot.board[1];
    const int b2 = snapshot.board[2];
    const int b3 = snapshot.board[3];
    const int b4 = snapshot.board[4];
    int bucket = -1;
    const auto& abstraction = strategy.get_abstraction();
    auto& current_state = table_data_[tournament_id].state;

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
    nlhe_state::holdem_action action = current_state->get_child(index)->get_action();

    // ensure the hand really terminates if our abstraction says so and we would just call
    if (action == nlhe_state::CALL
        && current_state->get_round() < holdem_state::RIVER
        && current_state->call()->is_terminal())
    {
        action = nlhe_state::RAISE_A;
        BOOST_LOG_TRIVIAL(info) << "Translating pre-river call to all-in to ensure hand terminates";
    }

    int next_action;
    double raise_fraction = -1;

    switch (action)
    {
    case nlhe_state::FOLD:
        next_action = table_manager::FOLD;
        break;
    case nlhe_state::CALL:
        next_action = table_manager::CALL;
        break;
    default:
        next_action = table_manager::RAISE;
        raise_fraction = current_state->get_raise_factor(action);
        break;
    }

    const QString s = current_state->get_action_name(action).c_str();
    const double probability = strategy.get_strategy().get_probability(*current_state, index, bucket);
    BOOST_LOG_TRIVIAL(info) << QString("Strategy: %1 (%2)").arg(s).arg(probability).toStdString();

    const auto& action_delay = *settings_->get_interval("action-delay");

    switch (next_action)
    {
    case table_manager::FOLD:
        BOOST_LOG_TRIVIAL(info) << "Folding";
        site_->fold(action_delay.second);
        break;
    case table_manager::CALL:
        BOOST_LOG_TRIVIAL(info) << "Calling";
        site_->call(action_delay.second);
        break;
    case table_manager::RAISE:
        {
            // we have to call opponent bet minus our bet
            const auto to_call = snapshot.bet[1] - snapshot.bet[0];

            // maximum bet is our remaining stack plus our bet (total raise to maxbet)
            const auto maxbet = snapshot.stack[0] + snapshot.bet[0];

            // minimum bet is opponent bet plus the amount we have to call (or big blind whichever is larger)
            // restrict minbet to always be less than or equal to maxbet (we can't bet more than stack)
            const auto minbet = std::min(snapshot.bet[1] + std::max(big_blind, to_call), maxbet);

            assert(minbet <= maxbet);

            // bet amount is opponent bet (our bet + call) plus x times the pot after our call (total_pot + to_call)
            const auto amount = boost::algorithm::clamp(
                snapshot.bet[1] + raise_fraction * (snapshot.total_pot + to_call), minbet, maxbet);

            const auto method = static_cast<table_manager::raise_method>(get_weighted_int(engine_,
                *settings_->get_number_list("bet-method-probabilities")));

            BOOST_LOG_TRIVIAL(info) << QString("Raising to %1 [%3, %5] (%2x pot) (method %4)").arg(amount)
                .arg(raise_fraction).arg(minbet).arg(method).arg(maxbet).toStdString();

            site_->raise(s.toStdString(), amount, minbet, action_delay.second, method);
        }
        break;
    }

    // update state pointer last after the table_manager input functions
    // this makes it possible to recover in case the buttons are stuck but become normal again as the state pointer
    // won't be in an invalid (terminal) state
    current_state = current_state->get_child(index);

    ENSURE(current_state != nullptr);
}

void main_window::handle_lobby()
{
    assert(lobby_);

    std::unordered_set<tid_t> active_tournaments;

    for (const auto& i : table_data_)
        active_tournaments.insert(i.first);

    lobby_->detect_closed_tables(active_tournaments);

    if (lobby_->is_registering()
        || (lobby_->get_registered_sngs() < table_count_->value()
            && (!schedule_action_->isChecked() || schedule_active_)))
    {
        lobby_->register_sng();
    }
}

void main_window::handle_schedule()
{
    if (!schedule_action_->isChecked())
        return;

    const auto active = is_schedule_active(*settings_->get_number("activity-span-variance"),
        *settings_->get_number("activity-day-variance"), settings_->get_activity_spans(), &time_to_next_activity_);

    if (active != schedule_active_)
    {
        schedule_active_ = active;

        if (schedule_active_)
        {
            BOOST_LOG_TRIVIAL(info) << "Enabling scheduled registration";
            lobby_->reset();
        }
        else
        {
            BOOST_LOG_TRIVIAL(info) << "Disabling scheduled registration";
        }

        BOOST_LOG_TRIVIAL(info) << QString("Next scheduled %1 in %2").arg(schedule_active_ ? "break" : "activity")
            .arg(secs_to_hms(time_to_next_activity_)).toStdString();
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
}

void main_window::update_strategy_widget(const tid_t tournament_id, const nlhe_strategy& strategy,
                                         const std::array<int, 2>& hole, const std::array<int, 5>& board)
{
    const auto state = table_data_[tournament_id].state;

    if (!state)
        return;

    strategy_widget_->set_hole(hole);
    strategy_widget_->set_board(board);
    strategy_widget_->update(strategy, *state);
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
        BOOST_LOG_TRIVIAL(info) << "Disabling scheduler";
}

void main_window::autolobby_changed(bool checked)
{
    if (checked)
    {
        BOOST_LOG_TRIVIAL(info) << "Enabling autolobby";
    }
    else
    {
        BOOST_LOG_TRIVIAL(info) << "Disabling autolobby";

        if (lobby_)
            lobby_->reset();
    }
}

void main_window::state_widget_state_changed()
{
    if (strategies_.empty() || !state_widget_->get_state())
        return;

    strategy_widget_->update(*strategies_.begin()->second, *state_widget_->get_state());
}

void main_window::update_statusbar()
{
    site_label_->setText(settings_->get_filename().c_str());
    strategy_label_->setText(QString("%1 strategies").arg(strategies_.size()));

    QString s;

    if (schedule_action_->isChecked())
    {
        if (schedule_active_)
            s = QString("Active for %1").arg(secs_to_hms(time_to_next_activity_));
        else
            s = QString("Inactive for %1").arg(secs_to_hms(time_to_next_activity_));
    }
    else
        s = "No schedule";

    schedule_label_->setText(s);

    registered_label_->setText(QString("Registrations: %1/%2 - Tables: %3/%2")
        .arg(lobby_ ? lobby_->get_registered_sngs() : 0)
        .arg(table_count_->value()).arg(visualizer_->rowCount()));
}

void main_window::remove_old_table_data()
{
    for (auto i = table_data_.begin(); i != table_data_.end();)
    {
        // it is possible to erroneously remove tables if many tables are performing actions during the same capture
        if (QDateTime::currentDateTime() >= i->second.timestamp.addMSecs(
            static_cast<int>(*settings_->get_number("tournament-prune-time") * 1000)))
        {
            BOOST_LOG_TRIVIAL(info) << "Removing table data for tournament " << i->first;

            visualizer_->remove(i->first);
            old_table_data_.erase(i->first);
            i = table_data_.erase(i);
        }
        else
            ++i;
    }
}

void main_window::load_settings(const std::string& filename)
{
    settings_->load(filename);

    const auto& input_delay = *settings_->get_interval("input-delay");
    const auto& double_click_delay = *settings_->get_interval("double-click-delay");
    const auto& mouse_speed = *settings_->get_interval("mouse-speed");

    input_manager_->set_delay(input_delay.first, input_delay.second);
    input_manager_->set_double_click_delay(double_click_delay.first, double_click_delay.second);
    input_manager_->set_mouse_speed(mouse_speed.first, mouse_speed.second);

    title_filter_->setText(settings_->get_string("title-filter")->c_str());
    table_count_->setValue(static_cast<int>(*settings_->get_number("table-count")));

    capture_timer_->setInterval(static_cast<int>(settings_->get_interval("capture")->first * 1000.0));

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
}

int main_window::get_effective_stack(const table_manager::snapshot_t& snapshot, const double big_blind) const
{
    // figure out the effective stack size

    // our stack size should always be visible
    ENSURE(snapshot.stack[0] > 0);

    std::array<double, 2> stacks;

    stacks[0] = snapshot.stack[0] + (snapshot.total_pot - snapshot.bet[0] - snapshot.bet[1]) / 2.0 + snapshot.bet[0];
    stacks[1] = *settings_->get_number("total-chips") - stacks[0];

    // both stacks sizes should be known by now
    ENSURE(stacks[0] > 0 && stacks[1] > 0);

    const auto stack_size = static_cast<int>(std::ceil(std::min(stacks[0], stacks[1]) / big_blind * 2.0));

    ENSURE(stack_size > 0);

    return stack_size;
}

bool main_window::is_new_game(const table_data_t& table_data, const table_manager::snapshot_t& snapshot) const
{
    // dealer button is not bugged and it has changed between snapshots -> new game
    // (dealer button status can change mid game on buggy clients)
    if (snapshot.dealer[0] != snapshot.dealer[1] &&
        table_data.snapshot.dealer[0] != table_data.snapshot.dealer[1] &&
        snapshot.dealer != table_data.snapshot.dealer)
    {
        BOOST_LOG_TRIVIAL(info) << "New game (dealer changed)";
        return true;
    }

    // hole cards changed -> new game
    if (snapshot.hole != table_data.snapshot.hole)
    {
        BOOST_LOG_TRIVIAL(info) << "New game (hole cards changed)";
        return true;
    }

    // stack size can never increase during a game between snapshots, so a new game must have started
    // only check if stack data is valid (not sitting out or all in)
    if ((snapshot.stack[0] > 0 && table_data.snapshot.stack[0] > 0 &&
            snapshot.stack[0] > table_data.snapshot.stack[0]) ||
        (snapshot.stack[1] > 0 && table_data.snapshot.stack[1] > 0 &&
            snapshot.stack[1] > table_data.snapshot.stack[1]))
    {
        BOOST_LOG_TRIVIAL(info) << "New game (stack increased)";
        return true;
    }

    // TODO: a false negatives can be possible in some corner cases
    return false;
}

void main_window::save_snapshot() const
{
    const auto& image = window_manager_->get_image();
    image.save(QDateTime::currentDateTimeUtc().toString("'snapshot-'yyyy-MM-ddTHHmmss.zzz'.png'"));
}
