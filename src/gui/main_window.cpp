#include "main_window.h"

#pragma warning(push, 1)
#include <array>
#include <fstream>
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
#include <Windows.h>
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

#include "cfrlib/holdem_game.h"
#include "cfrlib/nlhe_state.h"
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

#define ENSURE(x) ensure(x, #x, __LINE__)

namespace
{
    template<class T>
    typename T::const_iterator find_nearest(const T& map, const typename T::key_type& value)
    {
        const auto it = map.lower_bound(value);
        return it != map.end() ? it : boost::prior(map.end());
    }

    QString get_key_text(unsigned int vk)
    {
        const auto scancode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
        std::array<char, 100> s;

        if (GetKeyNameText(scancode << 16, s.data(), int(s.size())) != 0)
            return QString(s.data());
        else
            return "[Error]";
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
    autoplay_action_ = toolbar->addAction(QIcon(":/icons/control_play.png"), "Autoplay");
    autoplay_action_->setCheckable(true);
    connect(autoplay_action_, &QAction::toggled, this, &main_window::autoplay_changed);
    autolobby_action_ = toolbar->addAction(QIcon(":/icons/layout.png"), "Autolobby");
    autolobby_action_->setCheckable(true);
    connect(autolobby_action_, &QAction::toggled, this, &main_window::autolobby_changed);
    schedule_action_ = toolbar->addAction(QIcon(":/icons/time.png"), "Schedule");
    schedule_action_->setCheckable(true);
    connect(schedule_action_, &QAction::toggled, this, &main_window::schedule_changed);

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

    setWindowTitle("Window");
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

        find_capture_window();
        handle_schedule();

        if (lobby_)
        {
            lobby_->update_windows(capture_window_);

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
            site_->save_snapshot();
        }

        // ensure we can move after exceptions in case of visible tool tips disturbing scraping
        if (autolobby_action_->isChecked())
            input_manager_->move_random(input_manager::IDLE_MOVE_DESKTOP);
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
            lobby_.reset(new lobby_manager(*settings_, *input_manager_));
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
        BOOST_LOG_TRIVIAL(info) << "Starting autoplay";
    else
        BOOST_LOG_TRIVIAL(info) << "Stopping autoplay";
}

#pragma warning(push)
#pragma warning(disable: 4512)

void main_window::process_snapshot(const fake_window& window)
{
    if (!site_ && !lobby_)
        return;

    const auto tournament_id = lobby_->get_tournament_id(window);

    BOOST_LOG_SCOPED_THREAD_TAG("TournamentID", tournament_id);

    ENSURE(tournament_id != -1);

    auto& table_data = table_data_[tournament_id];
    table_data.timestamp = QDateTime::currentDateTime();

    const auto& snapshot = site_->update(window);

    if (save_images_->isChecked())
        site_->save_snapshot();

    // consider sitout fatal
    if (snapshot.sit_out[0])
    {
        if (table_count_->value() > 0)
        {
            BOOST_LOG_TRIVIAL(error) << "We are sitting out; stopping registrations";
            table_count_->setValue(0);

            BOOST_LOG_TRIVIAL(warning) << "Saving current snapshot";
            site_->save_snapshot();

            if (smtp_)
            {
                smtp_->send(settings_->get_string("smtp-from")->c_str(), settings_->get_string("stmp-to")->c_str(),
                    "[midas] " + QHostInfo::localHostName() + " needs attention", ".");
            }
        }

        return;
    }

    // these should work for observed tables
    visualizer_->clear_row(tournament_id);
    visualizer_->set_dealer(tournament_id, snapshot.dealer[0] ? 0 : (snapshot.dealer[1] ? 1 : -1));
    visualizer_->set_big_blind(tournament_id, snapshot.big_blind);
    visualizer_->set_real_pot(tournament_id, snapshot.total_pot);
    visualizer_->set_real_bets(tournament_id, snapshot.bet[0], snapshot.bet[1]);
    visualizer_->set_sit_out(tournament_id, snapshot.sit_out[0], snapshot.sit_out[1]);
    visualizer_->set_stacks(tournament_id, snapshot.stack[0], snapshot.stack[1]);
    visualizer_->set_buttons(tournament_id, snapshot.buttons);
    visualizer_->set_all_in(tournament_id, snapshot.all_in[0], snapshot.all_in[1]);

    int round = -1;

    visualizer_->set_hole_cards(tournament_id, snapshot.hole);
    visualizer_->set_board_cards(tournament_id, snapshot.board);

    if (snapshot.board[0] != -1 && snapshot.board[1] != -1 && snapshot.board[2] != -1)
    {
        if (snapshot.board[3] != -1)
        {
            if (snapshot.board[4] != -1)
                round = RIVER;
            else
                round = TURN;
        }
        else
        {
            round = FLOP;
        }
    }
    else if (snapshot.board[0] == -1 && snapshot.board[1] == -1 && snapshot.board[2] == -1)
    {
        round = PREFLOP;
    }
    else
    {
        round = -1;
    }

    // do not manipulate state when autoplay is off
    if (!autoplay_action_->isChecked())
        return;

    // wait until our box is highlighted
    if (!snapshot.highlight[0])
        return;

    // wait until we see buttons
    if (snapshot.buttons == 0)
        return;

    // wait for visible cards
    if (snapshot.hole[0] == -1 || snapshot.hole[1] == -1)
        return;

    // this will most likely fail if we can't read the cards
    ENSURE(round != -1);

    // make sure we read everything fine
    ENSURE(snapshot.big_blind != -1);
    ENSURE(snapshot.total_pot != -1);
    ENSURE(snapshot.bet[0] != -1);
    ENSURE(snapshot.bet[1] != -1);
    ENSURE(snapshot.dealer[0] || snapshot.dealer[1]);

    // wait until we see stack sizes
    if (snapshot.stack[0] == 0 || (snapshot.stack[1] == 0 && !snapshot.all_in[1] && !snapshot.sit_out[1]))
        return;

    // our stack size should always be visible
    ENSURE(snapshot.stack[0] > 0);
    // opponent stack might be obstructed by all-in or sitout statuses
    ENSURE(snapshot.stack[1] > 0 || snapshot.all_in[1] || snapshot.sit_out[1]);

    std::array<double, 2> stacks;

    stacks[0] = snapshot.stack[0] + (snapshot.total_pot - snapshot.bet[0] - snapshot.bet[1]) / 2.0
        + snapshot.bet[0];

    if (snapshot.sit_out[1])
    {
        stacks[1] = ALLIN_BET_SIZE; // can't see stack due to sitout, assume worst case
    }
    else
    {
        stacks[1] = snapshot.stack[1] + (snapshot.total_pot - snapshot.bet[0] - snapshot.bet[1]) / 2.0
            + snapshot.bet[1];
    }

    ENSURE(stacks[0] > 0 && stacks[1] > 0);

    const auto stack_size = int(std::ceil(std::min(stacks[0], stacks[1]) / snapshot.big_blind * 2.0));

    ENSURE(stack_size > 0);

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

    if (now < next_action_time)
        return;

    next_action_time = QDateTime();

    // wait until we see bet input
    const auto to_call = snapshot.bet[1] - snapshot.bet[0];
    const auto minbet = std::max(snapshot.big_blind, to_call) + to_call;

    if (!((snapshot.buttons & table_manager::INPUT_BUTTON) == table_manager::INPUT_BUTTON)
        && !snapshot.all_in[1]
        && !snapshot.sit_out[1]
        && snapshot.stack[0] > minbet)
    {
        BOOST_LOG_TRIVIAL(warning) << QString("Missing bet input when stack is greater than minbet (%1 > %2)")
            .arg(snapshot.stack[0]).arg(minbet).toStdString();
        return;
    }

    if (snapshot.hole == table_data.snapshot.hole
        && snapshot.board == table_data.snapshot.board
        && snapshot.stack == table_data.snapshot.stack
        && snapshot.bet == table_data.snapshot.bet)
    {
        BOOST_LOG_TRIVIAL(error) << "Identical snapshots; previous action not fulfilled; reverting state";
        table_data.state = table_data.initial_state;
    }

    table_data.initial_state = table_data.state;

    BOOST_LOG_TRIVIAL(info) << "*** SNAPSHOT ***";

    BOOST_LOG_TRIVIAL(info) << "Window: " << window.get_window_text();
    BOOST_LOG_TRIVIAL(info) << "Stack: " << stack_size << " SB";

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

    if (strategies_.empty())
        return;

    auto it = find_nearest(strategies_, stack_size);
    auto& strategy = *it->second;
    auto& current_state = table_data.state;

    BOOST_LOG_TRIVIAL(info) << QString("Strategy file: %1")
        .arg(QFileInfo(strategy.get_strategy().get_filename().c_str()).fileName()).toStdString();

    if (round == PREFLOP
        && ((snapshot.dealer[0] && snapshot.dealer[1]
            && (snapshot.dealer != table_data.snapshot.dealer || snapshot.hole != table_data.snapshot.hole))
        || (snapshot.dealer[0] && snapshot.bet[0] < snapshot.big_blind)
        || (snapshot.dealer[1] && snapshot.bet[0] == snapshot.big_blind)))
    {
        BOOST_LOG_TRIVIAL(info) << "New game";

        current_state = &strategy.get_root_state();

        if (snapshot.bet[0] < snapshot.big_blind)
            table_data.dealer = 0;
        else if (snapshot.bet[0] == snapshot.big_blind)
            table_data.dealer = 1;
        else
            table_data.dealer = -1;
    }

    const auto dealer = (snapshot.dealer[0] && snapshot.dealer[1])
        ? table_data.dealer
        : (snapshot.dealer[0] ? 0 : (snapshot.dealer[1] ? 1 : -1));

    if (snapshot.dealer[0] && snapshot.dealer[1])
        BOOST_LOG_TRIVIAL(warning) << "Multiple dealers";
    
    BOOST_LOG_TRIVIAL(info) << "Dealer is " << dealer;

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

    if ((current_state->get_round() == PREFLOP && (snapshot.bet[1] > snapshot.big_blind))
        || (current_state->get_round() > PREFLOP && (snapshot.bet[1] > 0)))
    {
        // make sure opponent allin is always terminal on his part and doesnt get translated to something else
        const double fraction = snapshot.all_in[1] ? ALLIN_BET_SIZE : (snapshot.bet[1] - snapshot.bet[0])
            / (snapshot.total_pot - (snapshot.bet[1] - snapshot.bet[0]));
        ENSURE(fraction > 0);
        BOOST_LOG_TRIVIAL(info) << QString("Opponent raised %1x pot").arg(fraction).toStdString();
        current_state = current_state->raise(fraction); // there is an outstanding bet/raise
    }
    else if (current_state->get_round() == PREFLOP && dealer == 1 && snapshot.bet[1] <= snapshot.big_blind)
    {
        BOOST_LOG_TRIVIAL(info) << "Facing big blind sized bet out of position preflop; opponent called";
        current_state = current_state->call();
    }
    else if (current_state->get_round() > PREFLOP && dealer == 0 && snapshot.bet[1] == 0)
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

    perform_action(tournament_id, strategy, snapshot);

    table_data.snapshot = snapshot;
}

#pragma warning(pop)

void main_window::perform_action(const tid_t tournament_id, const nlhe_strategy& strategy,
    const table_manager::snapshot_t& snapshot)
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

    const int index = (snapshot.sit_out[1] && current_state->get_child(nlhe_state_base::CALL + 1))
        ? nlhe_state_base::CALL + 1
        : strategy.get_strategy().get_action(current_state->get_id(), bucket);
    const nlhe_state_base::holdem_action action = current_state->get_action(index);
    const QString s = current_state->get_action_name(action).c_str();

    int next_action;
    double raise_fraction = -1;

    switch (action)
    {
    case nlhe_state_base::FOLD:
        next_action = table_manager::FOLD;
        break;
    case nlhe_state_base::CALL:
        {
            ENSURE(current_state->call() != nullptr);

            // ensure the hand really terminates if our abstraction says so
            if (current_state->get_round() < RIVER && current_state->call()->is_terminal())
            {
                next_action = table_manager::RAISE;
                raise_fraction = ALLIN_BET_SIZE;
                BOOST_LOG_TRIVIAL(info) << "Translating pre-river call to all-in to ensure hand terminates";
            }
            else
            {
                next_action = table_manager::CALL;
            }
        }
        break;
    default:
        next_action = table_manager::RAISE;
        raise_fraction = current_state->get_raise_factor(action);
        break;
    }

    const double probability = strategy.get_strategy().get(current_state->get_id(), index, bucket);
    BOOST_LOG_TRIVIAL(info) << QString("Strategy: %1 (%2)").arg(s).arg(probability).toStdString();

    current_state = current_state->get_child(index);

    ENSURE(current_state != nullptr);

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
            const double total_pot = snapshot.total_pot;
            const double my_bet = snapshot.bet[0];
            const double op_bet = snapshot.bet[1];
            const double to_call = op_bet - my_bet;
            const double amount = raise_fraction * (total_pot + to_call) + to_call + my_bet;
            const double minbet = std::max(snapshot.big_blind, to_call) + to_call + my_bet;
            const auto method = static_cast<table_manager::raise_method>(get_weighted_int(engine_,
                *settings_->get_number_list("bet-method-probabilities")));

            BOOST_LOG_TRIVIAL(info) << QString("Raising %1 (%2x pot) (%3 min) (method %4)").arg(amount).arg(raise_fraction)
                .arg(minbet).arg(method).toStdString();

            site_->raise(s.toStdString(), amount, minbet, action_delay.second, method);
        }
        break;
    }
}

void main_window::handle_lobby()
{
    assert(lobby_);

    std::unordered_set<tid_t> active_tournaments;

    for (const auto& i : table_data_)
        active_tournaments.insert(i.first);

    lobby_->detect_closed_tables(active_tournaments);

    if (lobby_->get_registered_sngs() < table_count_->value() && (!schedule_action_->isChecked() || schedule_active_))
        lobby_->register_sng();
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

void main_window::ensure(bool expression, const std::string& s, int line)
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
    if (!checked)
    {
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

void main_window::find_capture_window()
{
    const auto filter = title_filter_->text();

    if (filter.isEmpty())
        return;

    const auto window = FindWindow(nullptr, filter.toUtf8().data());

    if (IsWindow(window) && IsWindowVisible(window))
        capture_window_ = reinterpret_cast<WId>(window);
    else
        capture_window_ = 0;
}

void main_window::remove_old_table_data()
{
    for (auto i = table_data_.begin(); i != table_data_.end();)
    {
        if (QDateTime::currentDateTime() >= i->second.timestamp.addMSecs(
            static_cast<int>(*settings_->get_number("tournament-prune-time") * 1000)))
        {
            BOOST_LOG_TRIVIAL(info) << "Removing table data for tournament " << i->first;

            visualizer_->remove(i->first);
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
}
