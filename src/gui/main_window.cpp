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
#include <QSettings>
#include <QCoreApplication>
#include <QApplication>
#include <QSpinBox>
#include <QDateTime>
#pragma warning(pop)

#include "cfrlib/holdem_game.h"
#include "cfrlib/nlhe_state.h"
#include "cfrlib/strategy.h"
#include "util/card.h"
#include "cfrlib/nlhe_strategy.h"
#include "abslib/holdem_abstraction.h"
#include "util/random.h"
#include "table_widget.h"
#include "window_manager.h"
#include "holdem_strategy_widget.h"
#include "table_manager.h"
#include "input_manager.h"
#include "lobby_manager.h"
#include "state_widget.h"
#include "qt_log_sink.h"

#define ENSURE(x) ensure(x, #x, __LINE__)

namespace
{
    template<class T>
    typename T::const_iterator find_nearest(const T& map, const typename T::key_type& value)
    {
        auto upper = map.lower_bound(value);

        if (upper == map.begin() || (upper != map.end() && upper->first == value))
            return upper;

        auto lower = boost::prior(upper);

        if (upper == map.end() || (value - lower->first) < (upper->first - value))
            return lower;

        return upper;
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

    QString to_hms(int seconds)
    {
        const auto hours = seconds / 60 / 60;
        seconds -= hours * 60 * 60;

        const auto minutes = seconds / 60;
        seconds -= minutes * 60;

        return QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    }
}

main_window::main_window()
    : window_manager_(new window_manager)
    , engine_(std::random_device()())
    , input_manager_(new input_manager)
    , acting_(false)
    , break_active_(false)
    , schedule_active_(false)
{
    boost::log::add_common_attributes();

    static const char* log_format = "[%TimeStamp%] %Message%";

    boost::log::add_console_log
    (
        std::clog,
        boost::log::keywords::format = log_format
    );

    boost::log::add_file_log
    (
        boost::log::keywords::file_name = QDateTime::currentDateTimeUtc().toString("'log-'yyyyMMddTHHmmss'.txt'").toUtf8().data(),
        boost::log::keywords::auto_flush = true,
        boost::log::keywords::format = log_format
    );

    auto widget = new QWidget(this);
    widget->setFocus();

    setCentralWidget(widget);

    visualizer_ = new table_widget(this);
    strategy_ = new holdem_strategy_widget(this, Qt::Tool);
    strategy_->setVisible(false);
    state_widget_ = new state_widget(this, Qt::Tool);
    state_widget_->setVisible(false);
    connect(state_widget_, SIGNAL(board_changed(const QString&)), SLOT(state_widget_board_changed(const QString&)));
    connect(state_widget_, SIGNAL(state_reset()), SLOT(state_widget_state_reset()));
    connect(state_widget_, SIGNAL(called()), SLOT(state_widget_called()));
    connect(state_widget_, SIGNAL(raised(double)), SLOT(state_widget_raised(double)));

    auto toolbar = addToolBar("File");
    toolbar->setMovable(false);
    open_action_ = toolbar->addAction(QIcon(":/icons/folder_page_white.png"), "&Open...");
    open_action_->setIconText("Open...");
    open_action_->setToolTip("Open...");
    connect(open_action_, SIGNAL(triggered()), SLOT(open_strategy()));
    auto action = toolbar->addAction(QIcon(":/icons/map.png"), "Show strategy");
    connect(action, SIGNAL(triggered()), SLOT(show_strategy_changed()));
    action = toolbar->addAction(QIcon(":/icons/chart_organisation.png"), "Modify state");
    connect(action, SIGNAL(triggered()), SLOT(modify_state_changed()));
    save_images_ = toolbar->addAction(QIcon(":/icons/picture_save.png"), "Save images");
    save_images_->setCheckable(true);
    toolbar->addSeparator();

    title_filter_ = new QLineEdit(this);
    title_filter_->setPlaceholderText("Table title (regex)");
    connect(title_filter_, SIGNAL(textChanged(const QString&)), SLOT(table_title_changed(const QString&)));
    toolbar->addWidget(title_filter_);
    toolbar->addSeparator();
    lobby_title_ = new QLineEdit(this);
    lobby_title_->setPlaceholderText("Lobby title (exact)");
    connect(lobby_title_, SIGNAL(textChanged(const QString&)), SLOT(lobby_title_changed(const QString&)));
    toolbar->addWidget(lobby_title_);
    toolbar->addSeparator();
    table_count_ = new QSpinBox(this);
    table_count_->setValue(0);
    table_count_->setRange(0, 100);
    toolbar->addWidget(table_count_);
    toolbar->addSeparator();
    autoplay_action_ = toolbar->addAction(QIcon(":/icons/control_play.png"), "Autoplay");
    autoplay_action_->setCheckable(true);
    connect(autoplay_action_, SIGNAL(toggled(bool)), SLOT(play_changed(bool)));
    autolobby_action_ = toolbar->addAction(QIcon(":/icons/layout.png"), "Autolobby");
    autolobby_action_->setCheckable(true);
    connect(autolobby_action_, SIGNAL(toggled(bool)), SLOT(autolobby_changed(bool)));
    schedule_action_ = toolbar->addAction(QIcon(":/icons/time.png"), "Schedule");
    schedule_action_->setCheckable(true);
    connect(schedule_action_, SIGNAL(toggled(bool)), SLOT(schedule_changed(bool)));

    log_ = new QPlainTextEdit(this);
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(1000);

    typedef boost::log::sinks::synchronous_sink<qt_log_sink> sink_t;
    boost::shared_ptr<qt_log_sink> sink_backend(new qt_log_sink(log_));
    boost::shared_ptr<sink_t> sink_frontend(new sink_t(sink_backend));
    sink_frontend->set_formatter(boost::log::parse_formatter(log_format));
    boost::log::core::get()->add_sink(sink_frontend);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(visualizer_);
    layout->addWidget(log_);
    widget->setLayout(layout);

    capture_timer_ = new QTimer(this);
    connect(capture_timer_, SIGNAL(timeout()), SLOT(capture_timer_timeout()));
    autolobby_timer_ = new QTimer(this);
    connect(autolobby_timer_, SIGNAL(timeout()), SLOT(autolobby_timer_timeout()));
    break_timer_ = new QTimer(this);
    connect(break_timer_, SIGNAL(timeout()), SLOT(break_timer_timeout()));
    schedule_timer_ = new QTimer(this);
    connect(schedule_timer_, SIGNAL(timeout()), SLOT(schedule_timer_timeout()));
    registration_timer_ = new QTimer(this);
    registration_timer_->setSingleShot(true);
    connect(registration_timer_, SIGNAL(timeout()), SLOT(registration_timer_timeout()));

    capture_label_ = new QLabel("No window", this);
    statusBar()->addWidget(capture_label_, 1);
    schedule_label_ = new QLabel("Scheduler off", this);
    statusBar()->addWidget(schedule_label_, 1);
    registered_label_ = new QLabel("0/0", this);
    statusBar()->addWidget(registered_label_, 1);

    QSettings settings("settings.ini", QSettings::IniFormat);
    capture_interval_ = settings.value("capture-interval", 1).toDouble();
    action_delay_[0] = settings.value("action-delay-min", 0.5).toDouble();
    action_delay_[1] = settings.value("action-delay-max", 2.0).toDouble();
    action_post_delay_ = settings.value("post-action-delay", 1.0).toDouble();
    input_manager_->set_delay(settings.value("input-delay-min", 0.5).toDouble(),
        settings.value("input-delay-max", 1.0).toDouble());
    lobby_interval_ = settings.value("lobby-interval", 1.0).toDouble();
    const auto autolobby_hotkey = settings.value("autolobby-hotkey", VK_F1).toInt();
    day_start_ = settings.value("day-start", 10).toInt();
    day_finish_ = settings.value("day-finish", 20).toInt();
    break_interval_[0] = settings.value("break-interval-min", 60).toDouble();
    break_interval_[1] = settings.value("break-interval-max", 60).toDouble();
    break_length_[0] = settings.value("break-length-min", 10).toDouble();
    break_length_[0] = settings.value("break-length-max", 10).toDouble();
    input_manager_->set_mouse_speed(settings.value("mouse-speed-min", 1.5).toDouble(),
        settings.value("mouse-speed-max", 2.5).toDouble());

    log(QString("Loaded settings from \"%1\"").arg(settings.fileName()));

    if (RegisterHotKey(reinterpret_cast<HWND>(winId()), 0, MOD_ALT | MOD_CONTROL, autolobby_hotkey))
    {
        BOOST_LOG_TRIVIAL(info) << QString("Registered autolobby hotkey Ctrl+Alt+%1")
            .arg(get_key_text(autolobby_hotkey)).toStdString();
    }
    else
    {
        BOOST_LOG_TRIVIAL(info) << QString("Unable to register hotkey Ctrl+Alt+%1")
            .arg(get_key_text(autolobby_hotkey)).toStdString();
    }

    BOOST_LOG_TRIVIAL(info) << "Starting capture";

    capture_timer_->start(int(capture_interval_ * 1000.0));
}

main_window::~main_window()
{
}

void main_window::capture_timer_timeout()
{
    try
    {
        find_window();
        process_snapshot();
    }
    catch (const std::exception& e)
    {
        BOOST_LOG_TRIVIAL(error) << "Exception: " << e.what();
    }
}

void main_window::open_strategy()
{
    const auto filenames = QFileDialog::getOpenFileNames(this, "Open", QString(), "All files (*.*);;Site setting files (*.xml);;Strategy files (*.str)");

    if (filenames.empty())
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    strategy_infos_.clear();

    std::regex r("([^-]+)-([^-]+)-[0-9]+\\.str");
    std::regex r_nlhe("nlhe\\.([a-z]+)\\.([0-9]+)");
    std::smatch m;
    std::smatch m_nlhe;

    for (auto i = filenames.begin(); i != filenames.end(); ++i)
    {
        if (QFileInfo(*i).suffix() == "xml")
        {
            const auto filename = i->toStdString();
            lobby_.reset(new lobby_manager(filename, *input_manager_));
            site_.reset(new table_manager(filename, *input_manager_));
            log(QString("Loaded site settings \"%1\"").arg(filename.c_str()));
            continue;
        }

        const std::string filename = QFileInfo(*i).fileName().toStdString();

        try
        {
            std::unique_ptr<nlhe_strategy> p(new nlhe_strategy(i->toStdString()));
            auto& si = strategy_infos_[p->get_stack_size()];
            si.reset(new strategy_info);
            si->strategy_ = std::move(p);
        }
        catch (const std::runtime_error&)
        {
            continue;
        }

        log(QString("Loaded strategy \"%1\"").arg(*i));
    }

    QApplication::restoreOverrideCursor();
}

void main_window::show_strategy_changed()
{
    strategy_->setVisible(!strategy_->isVisible());
}

void main_window::autoplay_changed(const bool checked)
{
    if (checked)
    {
        log("Autoplay: Starting");
    }
    else
    {
        log("Autoplay: Stopping");
        acting_ = false;
    }
}

void main_window::action_start_timeout()
{
    log("Autoplay: Waiting for mutex...");

    auto mutex = window_manager_->try_interact();

    switch (next_action_)
    {
    case table_manager::FOLD:
        log("Player: Fold");
        site_->fold();
        break;
    case table_manager::CALL:
        log("Player: Call");
        site_->call();
        break;
    case table_manager::RAISE:
        {
            const double total_pot = site_->get_total_pot();
            const double my_bet = site_->get_bet(0);
            const double op_bet = site_->get_bet(1);
            const double to_call = op_bet - my_bet;
            const double amount = raise_fraction_ * (total_pot + to_call) + to_call + my_bet;

            log(QString("Autoplay: Raise %1 (%2x pot)").arg(amount).arg(raise_fraction_));
            site_->raise(amount, raise_fraction_);
        }
        break;
    }

    log(QString("Autoplay: Waiting for %1 s after action...").arg(action_post_delay_));
    QTimer::singleShot(int(action_post_delay_ * 1000.0), this, SLOT(action_finish_timeout()));
}

void main_window::find_window()
{
    if (site_)
        site_->set_window(window_manager_->get_window());

    if (window_manager_->is_window())
        return;

    WId window = 0;

    if (window_manager_->find_window())
    {
        window = window_manager_->get_window();

        const std::string title_name = window_manager_->get_title_name();
        capture_label_->setText(QString("%1").arg(title_name.c_str()));

        snapshot_.round = -1;
        snapshot_.bet = -1;
    }
    else
    {
        capture_label_->setText("No window");
    }
}

void main_window::process_snapshot()
{
    if (acting_ || !site_ || !site_->is_window())
        return;

    site_->update(save_images_->isChecked());

    const auto hole = site_->get_hole_cards();
    std::array<int, 5> board;
    site_->get_board_cards(board);

    int round = -1;

    if (hole.first != -1 && hole.second != -1)
    {
        if (board[0] != -1 && board[1] != -1 && board[2] != -1)
        {
            if (board[3] != -1)
            {
                if (board[4] != -1)
                    round = RIVER;
                else
                    round = TURN;
            }
            else
            {
                round = FLOP;
            }
        }
        else if (board[0] == -1 && board[1] == -1 && board[2] == -1)
        {
            round = PREFLOP;
        }
        else
        {
            round = -1;
        }
    }
    else
    {
        round = -1;
    }

    const bool new_game = round == 0
        && ((site_->get_dealer() == 0 && site_->get_bet(0) < site_->get_big_blind())
        || (site_->get_dealer() == 1 && site_->get_bet(0) == site_->get_big_blind()));

    visualizer_->set_dealer(site_->get_dealer());
    std::array<int, 2> hole_array = {{ hole.first, hole.second }};
    visualizer_->set_hole_cards(0, hole_array);
    visualizer_->set_board_cards(board);
    visualizer_->set_big_blind(site_->get_big_blind());
    visualizer_->set_real_pot(site_->get_total_pot());
    visualizer_->set_real_bets(site_->get_bet(0), site_->get_bet(1));
    visualizer_->set_sit_out(site_->is_sit_out(0), site_->is_sit_out(1));
    visualizer_->set_stacks(site_->get_stack(0), site_->get_stack(1));
    visualizer_->set_buttons(site_->get_buttons());

    if (!autoplay_action_->isChecked())
        return;

    if (site_->is_sit_out(0))
    {
        log("Warning: We are sitting out");

        if (autoplay_action_->isChecked())
        {
            log("Player: Sitting in");
            auto mutex = window_manager_->try_interact();
            site_->sit_in();
        }
    }

    // wait until we see buttons
    if (site_->get_buttons() == 0)
        return;

    // this will most likely fail if we can't read the cards
    ENSURE(round != -1);

    // wait until we see stack sizes
    if (site_->get_stack(0) == 0 || (site_->get_stack(1) == 0 && !site_->is_opponent_allin() && !site_->is_opponent_sitout()))
        return;

    snapshot_.round = round;
    snapshot_.bet = site_->get_bet(1);

    // our stack size should always be visible
    ENSURE(site_->get_stack(0) > 0);
    // opponent stack might be obstructed by all-in or sitout statuses
    ENSURE(site_->get_stack(1) > 0 || site_->is_opponent_allin() || site_->is_opponent_sitout());

    if (new_game)
    {
        std::array<double, 2> stacks;
        
        stacks[0] = site_->get_stack(0) + site_->get_bet(0);

        if (site_->get_stack(1) > 0)
        {
            stacks[1] = site_->get_stack(1) + site_->get_bet(1); // opponent stack equals stack+bet
        }
        else
        {
            if (site_->is_opponent_sitout())
                stacks[1] = ALLIN_BET_SIZE; // can't see stack due to sitout, assume worst case
            else if (site_->is_opponent_allin())
                stacks[1] = site_->get_bet(1); // opponent stack equals his bet
        }

        ENSURE(stacks[0] > 0 && stacks[1] > 0);
        snapshot_.stack_size = int(std::min(stacks[0], stacks[1]) / site_->get_big_blind() * 2 + 0.5);
    }

    BOOST_LOG_TRIVIAL(info) << "*** SNAPSHOT ***";

    if (hole.first != -1 && hole.second != -1)
    {
        log(QString("Hole: [%1 %2]").arg(get_card_string(hole.first).c_str())
           .arg(get_card_string(hole.second).c_str()));
    }

    if (board[4] != -1)
    {
        log(QString("Board: [%1 %2 %3 %4] [%5]").arg(get_card_string(board[0]).c_str()).arg(get_card_string(board[1]).c_str())
            .arg(get_card_string(board[2]).c_str()).arg(get_card_string(board[3]).c_str()).arg(get_card_string(board[4]).c_str()));
    }
    else if (board[3] != -1)
    {
        log(QString("Board: [%1 %2 %3] [%4]").arg(get_card_string(board[0]).c_str()).arg(get_card_string(board[1]).c_str())
            .arg(get_card_string(board[2]).c_str()).arg(get_card_string(board[3]).c_str()));
    }
    else if (board[0] != -1)
    {
        log(QString("Board: [%1 %2 %3]").arg(get_card_string(board[0]).c_str()).arg(get_card_string(board[1]).c_str())
            .arg(get_card_string(board[2]).c_str()));
    }

    if (strategy_infos_.empty())
        return;

    auto it = find_nearest(strategy_infos_, snapshot_.stack_size);
    auto& strategy_info = *it->second;
    auto& current_state = strategy_info.current_state_;

    if (new_game && round == 0)
    {
        log(QString("State: New game (%1 SB)").arg(snapshot_.stack_size));
        current_state = &strategy_info.strategy_->get_root_state();
    }

    if (!current_state)
    {
        log("Warning: Invalid state");
        return;
    }

    // a new round has started but we did not end the previous one, opponent has called
    if (current_state->get_round() != round)
    {
        log("State: Opponent called");
        current_state = current_state->call();
    }

    // note: we modify the current state normally as this will be interpreted as a check
    if (site_->is_opponent_sitout())
        log("State: Opponent is sitting out");

    if (site_->is_opponent_allin())
        log("State: Opponent is all-in");

    if ((current_state->get_round() == PREFLOP && (site_->get_bet(1) > site_->get_big_blind()))
        || (current_state->get_round() > PREFLOP && (site_->get_bet(1) > 0)))
    {
        // make sure opponent allin is always terminal on his part and doesnt get translated to something else
        const double fraction = site_->is_opponent_allin() ? ALLIN_BET_SIZE : (site_->get_bet(1) - site_->get_bet(0))
            / (site_->get_total_pot() - (site_->get_bet(1) - site_->get_bet(0)));
        ENSURE(fraction > 0);
        log(QString("State: Opponent raised %1x pot").arg(fraction));
        current_state = current_state->raise(fraction); // there is an outstanding bet/raise
    }
    else if ((current_state->get_round() == PREFLOP && site_->get_dealer() == 1 && site_->get_bet(1) <= site_->get_big_blind())
        || (current_state->get_round() > PREFLOP && site_->get_dealer() == 0 && site_->get_bet(1) == 0))
    {
        // we are in position facing 0 sized bet, opponent has checked
        // we are oop facing big blind sized bet preflop, opponent has called
        log("State: Opponent called");
        current_state = current_state->call();
    }

    ENSURE(current_state != nullptr);

    // ensure it is our turn
    ENSURE((site_->get_dealer() == 0 && current_state->get_player() == 0)
            || (site_->get_dealer() == 1 && current_state->get_player() == 1));

    // ensure rounds match
    ENSURE(current_state->get_round() == round);

    std::array<int, 2> pot = current_state->get_pot();
        
    if (site_->get_dealer() == 1)
        std::swap(pot[0], pot[1]);

    visualizer_->set_pot(current_state->get_round(), pot);

    // we should never reach terminal states when we have a pending action
    ENSURE(current_state->get_id() != -1);

    std::stringstream ss;
    ss << *current_state;
    log(QString("State: %1").arg(ss.str().c_str()));

    update_strategy_widget(strategy_info);

    perform_action();
}

void main_window::perform_action()
{
    ENSURE(site_ && !strategy_infos_.empty());

    auto it = find_nearest(strategy_infos_, snapshot_.stack_size);
    auto& strategy_info = *it->second;

    auto hole = site_->get_hole_cards();
    std::array<int, 5> board;
    site_->get_board_cards(board);

    const int c0 = hole.first;
    const int c1 = hole.second;
    const int b0 = board[0];
    const int b1 = board[1];
    const int b2 = board[2];
    const int b3 = board[3];
    const int b4 = board[4];
    int bucket = -1;
    const auto& abstraction = strategy_info.strategy_->get_abstraction();

    if (c0 != -1 && c1 != -1)
    {
        holdem_abstraction_base::bucket_type buckets;
        abstraction.get_buckets(c0, c1, b0, b1, b2, b3, b4, &buckets);
        bucket = buckets[strategy_info.current_state_->get_round()];
    }

    auto& current_state = strategy_info.current_state_;

    ENSURE(current_state && !current_state->is_terminal() && bucket != -1);

    const auto& strategy = strategy_info.strategy_;
    const int index = site_->is_opponent_sitout() ? nlhe_state_base::CALL + 1
        : strategy->get_strategy().get_action(current_state->get_id(), bucket);
    const int action = current_state->get_action(index);
    const QString s = current_state->get_action_name(action).c_str();

    switch (action)
    {
    case nlhe_state_base::FOLD:
        next_action_ = table_manager::FOLD;
        break;
    case nlhe_state_base::CALL:
        {
            ENSURE(current_state->call() != nullptr);

            // ensure the hand really terminates if our abstraction says so
            if (current_state->call()->is_terminal())
            {
                next_action_ = table_manager::RAISE;
                raise_fraction_ = ALLIN_BET_SIZE;
                BOOST_LOG_TRIVIAL(info) << "Strategy: Translating call to all-in to ensure hand terminates";
            }
            else
            {
                next_action_ = table_manager::CALL;
            }
        }
        break;
    default:
        next_action_ = table_manager::RAISE;
        raise_fraction_ = current_state->get_raise_factor(action);
        break;
    }

    const double probability = strategy->get_strategy().get(current_state->get_id(), index, bucket);
    log(QString("Strategy: %1 (%2)").arg(s).arg(probability));

    current_state = current_state->get_child(index);

    acting_ = true;

    assert(autoplay_action_->isChecked());

    const double wait = site_->is_opponent_sitout() ? action_delay_[0] : get_normal_random(engine_,
        action_delay_[0], action_delay_[1]);

    QTimer::singleShot(static_cast<int>(wait * 1000.0), this, SLOT(action_start_timeout()));

    BOOST_LOG_TRIVIAL(info) << QString("Autoplay: Waiting %1 s before action").arg(wait).toStdString();
}

main_window::strategy_info::strategy_info()
    : current_state_(nullptr)
{
}

main_window::strategy_info::~strategy_info()
{
}

void main_window::action_finish_timeout()
{
    acting_ = false;
}

void main_window::autolobby_timer_timeout()
{
    if (window_manager_->is_stop())
    {
        BOOST_LOG_TRIVIAL(warning) << "Autolobby: Stopping due to global stop";
        autolobby_action_->setChecked(false);
        return;
    }

    if (!lobby_)
        return;

    registered_label_->setText(QString("%1/%2").arg(lobby_->get_registered_sngs()).arg(table_count_->value()));

    if (!lobby_->is_window())
    {
        const auto filter = lobby_title_->text();

        if (filter.isEmpty())
            return;

        const auto window = FindWindow(nullptr, filter.toUtf8().data());

        if (!IsWindow(window))
            return;

        lobby_->set_window(reinterpret_cast<WId>(window));
    }

    assert(lobby_ && lobby_->is_window());

    auto mutex = window_manager_->try_interact();

    //log(QString("Registered in %1 tournaments").arg(lobby_->get_registered_sngs()));

    if (!lobby_->ensure_visible())
        return;

    if (lobby_->get_registered_sngs() < table_count_->value()
        && (!schedule_action_->isChecked() || (schedule_active_ && !break_active_)))
    {
        if (lobby_->register_sng())
        {
            const int wait = 5000;
            BOOST_LOG_TRIVIAL(info) << "Waiting " << wait << " ms for registration to complete";
            registration_timer_->start(wait);
        }
    }

    const auto old = lobby_->get_registered_sngs();

    if (lobby_->close_popups())
    {
        BOOST_LOG_TRIVIAL(info) << "Stopping registration timeout timer";
        registration_timer_->stop();
    }

    if (lobby_->get_registered_sngs() != old)
        log(QString("Lobby: Registration count changed (%1 -> %2)").arg(old).arg(lobby_->get_registered_sngs()));
}

void main_window::log(const QString& s)
{
    BOOST_LOG_TRIVIAL(info) << s.toStdString();
}

bool main_window::nativeEvent(const QByteArray& /*eventType*/, void* message, long* /*result*/)
{
    if (reinterpret_cast<MSG*>(message)->message == WM_HOTKEY)
    {
        autolobby_action_->trigger();
        return true;
    }

    return false;
}

void main_window::modify_state_changed()
{
    state_widget_->setVisible(!state_widget_->isVisible());
}

void main_window::state_widget_board_changed(const QString& board)
{
    std::string s(board.toStdString());
    std::array<int, 5> b;
    b.fill(-1);

    for (auto i = 0; i < b.size(); ++i)
    {
        if (s.size() < 2)
            break;

        b[i] = string_to_card(s);
        s.erase(s.begin(), s.begin() + 2);
    }

    visualizer_->set_board_cards(b);

    if (!strategy_infos_.empty())
        update_strategy_widget(*strategy_infos_.begin()->second);
}

void main_window::update_strategy_widget(const strategy_info& si)
{
    if (!si.current_state_)
        return;

    std::array<int, 2> hole;
    visualizer_->get_hole_cards(hole);

    std::array<int, 5> board;
    visualizer_->get_board_cards(board);

    switch (si.current_state_->get_round())
    {
    case PREFLOP:
        board[0] = -1;
        board[1] = -1;
        board[2] = -1;
    case FLOP:
        board[3] = -1;
    case TURN:
        board[4] = -1;
    }

    strategy_->update(*si.strategy_, hole, board, si.current_state_->get_id());

    std::stringstream ss;
    ss << *si.current_state_;

    state_widget_->set_state(ss.str().c_str());
}

void main_window::state_widget_state_reset()
{
    if (strategy_infos_.empty())
        return;

    auto& si = *strategy_infos_.begin()->second;
    si.current_state_ = &si.strategy_->get_root_state();
    update_strategy_widget(si);
}

void main_window::state_widget_called()
{
    if (strategy_infos_.empty())
        return;

    auto& si = *strategy_infos_.begin()->second;

    if (!si.current_state_)
        return;

    const auto state = si.current_state_->call();

    if (state->is_terminal())
        return;

    si.current_state_ = state;
    update_strategy_widget(si);
}

void main_window::state_widget_raised(double fraction)
{
    if (strategy_infos_.empty())
        return;

    auto& si = *strategy_infos_.begin()->second;

    if (!si.current_state_)
        return;
 
    si.current_state_ = si.current_state_->raise(fraction);
    update_strategy_widget(si);
}

void main_window::ensure(bool expression, const std::string& s, int line)
{
    if (expression)
        return;

    assert(false);
    log(QString("Error: verification on line %1 failed (%2)").arg(line).arg(s.c_str()));
    window_manager_->stop();

    BOOST_LOG_TRIVIAL(info) << "Saving current snapshot";

    site_->save_snapshot();

    throw std::runtime_error(s.c_str());
}

void main_window::schedule_changed(const bool checked)
{
    if (checked)
    {
        log("Scheduler: Enabling");
        schedule_timer_->setInterval(1000);
        schedule_timer_->start();
    }
    else
    {
        log("Scheduler: Disabling");
        schedule_timer_->stop();
        break_timer_->stop();
        schedule_active_ = false;
    }

    schedule_label_->setText(checked ? "Scheduler on" : "Scheduler off");
}

void main_window::break_timer_timeout()
{
    if (!break_active_)
    {
        break_active_ = true;

        const auto duration = get_normal_random(engine_, break_length_[0], break_length_[1]);

        log(QString("Scheduler: Break for %1").arg(to_hms(static_cast<int>(duration * 60))));

        break_timer_->setSingleShot(true);
        break_timer_->start(int(duration * 60.0 * 1000.0));
    }
    else
    {
        break_active_ = false;

        const auto duration = get_normal_random(engine_, break_interval_[0], break_interval_[1]);

        log(QString("Scheduler: Next break in %1 minutes").arg(to_hms(static_cast<int>(duration * 60))));

        break_timer_->setSingleShot(true);
        break_timer_->start(int(duration * 60.0 * 1000.0));
    }
}

void main_window::schedule_timer_timeout()
{
    const auto now = QDateTime::currentDateTime();
    const auto hour = now.toString("H").toInt();
    const bool active = (hour >= day_start_ && hour < day_finish_);

    if (active && !schedule_active_)
    {
        log(QString("Scheduler: Enabling registration: hour is %1 [%2, %3]").arg(hour).arg(day_start_)
            .arg(day_finish_));

        schedule_active_ = true;
        break_active_ = true; // make sure we don't start with a break

        break_timer_->start();
    }
    else if (!active && schedule_active_)
    {
        log(QString("Scheduler: Disabling registration: hour is %1 [%2, %3]").arg(hour).arg(day_start_)
            .arg(day_finish_));

        schedule_active_ = false;
        break_active_ = false;

        break_timer_->stop();
    }

    QString s;

    if (active)
    {
        if (break_active_)
            s = QString("Break for %1").arg(to_hms(break_timer_->remainingTime() / 1000));
        else
            s = QString("Next break in %1").arg(to_hms(break_timer_->remainingTime() / 1000));
    }
    else
        s = QString("Inactive until: %1:00-%2:00").arg(day_start_, 2).arg(day_finish_, 2);

    schedule_label_->setText(s);
}

void main_window::registration_timer_timeout()
{
    lobby_->cancel_registration();
}

void main_window::table_title_changed(const QString& str)
{
    window_manager_->set_title_filter(str.toStdString());
}

void main_window::autolobby_changed(bool checked)
{
    if (checked)
    {
        autolobby_timer_->start(int(lobby_interval_ * 1000.0));
    }
    else
    {
        autolobby_timer_->stop();
        lobby_->reset();
    }
}
