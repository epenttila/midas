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
#include <boost/log/utility/setup/from_stream.hpp>
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
#include <QMessageBox>
#include <QHostInfo>
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

    double datetime_to_secs(const QDateTime& dt)
    {
        return dt.toString("H").toInt() * 3600 + dt.toString("m").toInt() * 60 + dt.toString("s").toInt()
            + dt.toString("z").toInt() / 1000.0;
    }

    double hms_to_secs(const QString& str)
    {
        return datetime_to_secs(QDateTime::fromString(str, "HH:mm:ss"));
    }

    bool is_schedule_active(const double var, const main_window::spans_t& daily_spans, double* time)
    {
        assert(daily_spans.size() == 7);

        const auto t = QDateTime::currentDateTime();
        const auto day = t.date().dayOfWeek() - 1;
        const auto& spans = daily_spans[day];
        const auto now = datetime_to_secs(t);

        std::mt19937_64 engine(t.date().toJulianDay());

        for (const auto span : spans)
        {
            const auto first = get_uniform_random(engine, span.first - var / 2.0, span.first + var / 2.0);
            const auto second = get_uniform_random(engine, span.second - var / 2.0, span.second + var / 2.0);

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

    try
    {
        std::ifstream file("settings.ini");
        boost::log::init_from_stream(file);
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, "Exception", e.what());
        BOOST_LOG_TRIVIAL(error) << e.what();
    }

    QSettings settings("settings.ini", QSettings::IniFormat);
    capture_interval_ = settings.value("capture-interval", 1).toDouble();
    action_delay_[0] = settings.value("action-delay-min", 0.5).toDouble();
    action_delay_[1] = settings.value("action-delay-max", 2.0).toDouble();
    input_manager_->set_delay(settings.value("input-delay-min", 0.5).toDouble(),
        settings.value("input-delay-max", 1.0).toDouble());
    lobby_interval_ = settings.value("lobby-interval", 1.0).toDouble();
    const auto autolobby_hotkey = settings.value("autolobby-hotkey", VK_F1).toInt();
    input_manager_->set_mouse_speed(settings.value("mouse-speed-min", 1.5).toDouble(),
        settings.value("mouse-speed-max", 2.5).toDouble());
    activity_variance_ = settings.value("activity-variance", 0.0).toDouble();
    title_filter_->setText(settings.value("title-filter").toString());

    for (int day = 0; day < 7; ++day)
    {
        for (auto i : settings.value(QString("activity-spans-%1").arg(day)).toStringList())
        {
            if (i == "0")
                continue;

            const auto j = i.split(QChar('-'));

            if (j.size() != 2)
            {
                BOOST_LOG_TRIVIAL(error) << "Invalid activity span setting";
                continue;
            }

            activity_spans_[day].push_back(std::make_pair(hms_to_secs(j.at(0)), hms_to_secs(j.at(1))));
        }
    }

    BOOST_LOG_TRIVIAL(info) << QString("Loaded program settings: %1").arg(settings.fileName()).toStdString();

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

    const auto smtp_host = settings.value("smtp-host").toString();
    const auto smtp_port = static_cast<std::uint16_t>(settings.value("smtp-port").toUInt());
    smtp_from_ = settings.value("smtp-from").toString();
    smtp_to_ = settings.value("smtp-to").toString();

    if (!smtp_host.isEmpty())
    {
        smtp_ = new smtp(smtp_host, smtp_port);

        connect(smtp_, &smtp::status, [](const QString& s)
        {
            BOOST_LOG_TRIVIAL(info) << s.toStdString();
        });
    }
    else
        smtp_ = nullptr;

    BOOST_LOG_TRIVIAL(info) << "Starting capture";

    capture_timer_->start(int(capture_interval_ * 1000.0));

    setWindowTitle("Window");
}

main_window::~main_window()
{
}

void main_window::capture_timer_timeout()
{
    try
    {
        find_capture_window();

        if (lobby_)
        {
            visualizer_->set_tables(static_cast<int>(lobby_->get_tables().size()));

            lobby_->update_windows(capture_window_);

            handle_lobby();

            for (const auto& window : lobby_->get_tables())
            {
                if (window->is_valid())
                    process_snapshot(*window);
            }
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

        if (smtp_)
            smtp_->send(smtp_from_, smtp_to_, "[midas] Fatal error on " + QHostInfo::localHostName(), e.what());
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
            lobby_.reset(new lobby_manager(filename, *input_manager_));
            site_.reset(new table_manager(filename, *input_manager_));

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
    if (!site_)
        return;

    const auto now = QDateTime::currentDateTime();

    if (!next_action_times_.count(window.get_id()))
        next_action_times_[window.get_id()] = now;
    else if (now < next_action_times_[window.get_id()])
        return;

    site_->update(window);

    if (save_images_->isChecked())
        site_->save_snapshot();

    // consider sitout fatal
    if (site_->is_sit_out(0))
    {
        if (table_count_->value() > 0)
        {
            BOOST_LOG_TRIVIAL(error) << "We are sitting out; stopping registrations";
            table_count_->setValue(0);

            BOOST_LOG_TRIVIAL(warning) << "Saving current snapshot";
            site_->save_snapshot();
        }

        return;
    }

    // these should work for observed tables
    visualizer_->clear_row(window.get_id());
    visualizer_->set_active(window.get_id());
    visualizer_->set_dealer(window.get_id(), site_->get_dealer());
    visualizer_->set_big_blind(window.get_id(), site_->get_big_blind());
    visualizer_->set_real_pot(window.get_id(), site_->get_total_pot());
    visualizer_->set_real_bets(window.get_id(), site_->get_bet(0), site_->get_bet(1));
    visualizer_->set_sit_out(window.get_id(), site_->is_sit_out(0), site_->is_sit_out(1));
    visualizer_->set_stacks(window.get_id(), site_->get_stack(0), site_->get_stack(1));
    visualizer_->set_buttons(window.get_id(), site_->get_buttons());
    visualizer_->set_all_in(window.get_id(), site_->is_all_in(0), site_->is_all_in(1));

    int round = -1;
    std::array<int, 5> board;

    // read board only when autoplay is off (observed tables) or we see buttons (our turn)
    if (!autoplay_action_->isChecked() || site_->get_buttons() != 0)
    {
        site_->get_board_cards(board);
        visualizer_->set_board_cards(window.get_id(), board);

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

    // do not manipulate state when autoplay is off
    if (!autoplay_action_->isChecked())
        return;

    // wait until we see buttons
    if (site_->get_buttons() == 0)
        return;

    // read hole cards only when autoplay is on and we see buttons
    std::array<int, 2> hole;
    site_->get_hole_cards(hole);
    visualizer_->set_hole_cards(window.get_id(), hole);

    ENSURE(hole[0] != -1 && hole[1] != -1);

    // this will most likely fail if we can't read the cards
    ENSURE(round != -1);

    // make sure we read everything fine
    ENSURE(site_->get_big_blind() != -1);
    ENSURE(site_->get_total_pot() != -1);
    ENSURE(site_->get_bet(0) != -1);
    ENSURE(site_->get_bet(1) != -1);
    ENSURE(site_->get_dealer() != -1);

    // wait until we see stack sizes
    if (site_->get_stack(0) == 0 || (site_->get_stack(1) == 0 && !site_->is_opponent_allin() && !site_->is_opponent_sitout()))
        return;

    // our stack size should always be visible
    ENSURE(site_->get_stack(0) > 0);
    // opponent stack might be obstructed by all-in or sitout statuses
    ENSURE(site_->get_stack(1) > 0 || site_->is_opponent_allin() || site_->is_opponent_sitout());

    std::array<double, 2> stacks;

    stacks[0] = site_->get_stack(0) + (site_->get_total_pot() - site_->get_bet(0) - site_->get_bet(1)) / 2.0
        + site_->get_bet(0);

    if (site_->is_opponent_sitout())
    {
        stacks[1] = ALLIN_BET_SIZE; // can't see stack due to sitout, assume worst case
    }
    else
    {
        stacks[1] = site_->get_stack(1) + (site_->get_total_pot() - site_->get_bet(0) - site_->get_bet(1)) / 2.0
            + site_->get_bet(1);
    }

    ENSURE(stacks[0] > 0 && stacks[1] > 0);

    const auto stack_size = int(std::ceil(std::min(stacks[0], stacks[1]) / site_->get_big_blind() * 2.0));

    ENSURE(stack_size > 0);

    BOOST_LOG_TRIVIAL(info) << "*** SNAPSHOT ***";

    BOOST_LOG_TRIVIAL(info) << "Window: " << window.get_id() << " (" << window.get_window_text() << ")";
    BOOST_LOG_TRIVIAL(info) << "Stack: " << stack_size << " SB";

    if (hole[0] != -1 && hole[1] != -1)
    {
        BOOST_LOG_TRIVIAL(info) << QString("Hole: [%1 %2]").arg(get_card_string(hole[0]).c_str())
            .arg(get_card_string(hole[1]).c_str()).toStdString();
    }

    QString board_string;

    if (board[4] != -1)
    {
        board_string = QString("Board: [%1 %2 %3 %4] [%5]").arg(get_card_string(board[0]).c_str())
            .arg(get_card_string(board[1]).c_str()).arg(get_card_string(board[2]).c_str())
            .arg(get_card_string(board[3]).c_str()).arg(get_card_string(board[4]).c_str());
    }
    else if (board[3] != -1)
    {
        board_string = QString("Board: [%1 %2 %3] [%4]").arg(get_card_string(board[0]).c_str())
            .arg(get_card_string(board[1]).c_str()).arg(get_card_string(board[2]).c_str())
            .arg(get_card_string(board[3]).c_str());
    }
    else if (board[0] != -1)
    {
        board_string = QString("Board: [%1 %2 %3]").arg(get_card_string(board[0]).c_str())
            .arg(get_card_string(board[1]).c_str()).arg(get_card_string(board[2]).c_str());
    }

    if (!board_string.isEmpty())
        BOOST_LOG_TRIVIAL(info) << board_string.toStdString();

    if (strategies_.empty())
        return;

    auto it = find_nearest(strategies_, stack_size);
    auto& strategy = *it->second;
    auto& current_state = states_[window.get_id()];

    BOOST_LOG_TRIVIAL(info) << QString("Strategy file: %1")
        .arg(QFileInfo(strategy.get_strategy().get_filename().c_str()).fileName()).toStdString();

    const bool new_game = round == 0
        && ((site_->get_dealer() == 0 && site_->get_bet(0) < site_->get_big_blind())
        || (site_->get_dealer() == 1 && site_->get_bet(0) == site_->get_big_blind()));

    if (new_game && round == 0)
    {
        BOOST_LOG_TRIVIAL(info) << "New game";
        current_state = &strategy.get_root_state();
    }

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
    if (site_->is_opponent_sitout())
        BOOST_LOG_TRIVIAL(info) << "Opponent is sitting out";

    if (site_->is_opponent_allin())
        BOOST_LOG_TRIVIAL(info) << "Opponent is all-in";

    if ((current_state->get_round() == PREFLOP && (site_->get_bet(1) > site_->get_big_blind()))
        || (current_state->get_round() > PREFLOP && (site_->get_bet(1) > 0)))
    {
        // make sure opponent allin is always terminal on his part and doesnt get translated to something else
        const double fraction = site_->is_opponent_allin() ? ALLIN_BET_SIZE : (site_->get_bet(1) - site_->get_bet(0))
            / (site_->get_total_pot() - (site_->get_bet(1) - site_->get_bet(0)));
        ENSURE(fraction > 0);
        BOOST_LOG_TRIVIAL(info) << QString("Opponent raised %1x pot").arg(fraction).toStdString();
        current_state = current_state->raise(fraction); // there is an outstanding bet/raise
    }
    else if (current_state->get_round() == PREFLOP && site_->get_dealer() == 1 && site_->get_bet(1) <= site_->get_big_blind())
    {
        BOOST_LOG_TRIVIAL(info) << "Facing big blind sized bet out of position preflop; opponent called";
        current_state = current_state->call();
    }
    else if (current_state->get_round() > PREFLOP && site_->get_dealer() == 0 && site_->get_bet(1) == 0)
    {
        // we are in position facing 0 sized bet, opponent has checked
        // we are oop facing big blind sized bet preflop, opponent has called
        BOOST_LOG_TRIVIAL(info) << "Facing 0-sized bet in position postflop; opponent checked";
        current_state = current_state->call();
    }

    ENSURE(current_state != nullptr);

    // ensure it is our turn
    ENSURE((site_->get_dealer() == 0 && current_state->get_player() == 0)
            || (site_->get_dealer() == 1 && current_state->get_player() == 1));

    // ensure rounds match
    ENSURE(current_state->get_round() == round);

    // we should never reach terminal states when we have a pending action
    ENSURE(current_state->get_id() != -1);

    std::stringstream ss;
    ss << *current_state;
    BOOST_LOG_TRIVIAL(info) << QString("State: %1").arg(ss.str().c_str()).toStdString();

    update_strategy_widget(window, strategy, hole, board);

    perform_action(window, strategy);
}

#pragma warning(pop)

void main_window::perform_action(const fake_window& window, const nlhe_strategy& strategy)
{
    ENSURE(site_ && !strategies_.empty());

    std::array<int, 2> hole;
    site_->get_hole_cards(hole);
    std::array<int, 5> board;
    site_->get_board_cards(board);

    const int c0 = hole[0];
    const int c1 = hole[1];
    const int b0 = board[0];
    const int b1 = board[1];
    const int b2 = board[2];
    const int b3 = board[3];
    const int b4 = board[4];
    int bucket = -1;
    const auto& abstraction = strategy.get_abstraction();
    auto& current_state = states_[window.get_id()];

    ENSURE(current_state != nullptr);
    ENSURE(!current_state->is_terminal());

    if (c0 != -1 && c1 != -1)
    {
        holdem_abstraction_base::bucket_type buckets;
        abstraction.get_buckets(c0, c1, b0, b1, b2, b3, b4, &buckets);
        bucket = buckets[current_state->get_round()];
    }

    ENSURE(bucket != -1);

    const int index = site_->is_opponent_sitout() ? nlhe_state_base::CALL + 1
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

    switch (next_action)
    {
    case table_manager::FOLD:
        BOOST_LOG_TRIVIAL(info) << "Folding";
        site_->fold(action_delay_[1]);
        break;
    case table_manager::CALL:
        BOOST_LOG_TRIVIAL(info) << "Calling";
        site_->call(action_delay_[1]);
        break;
    case table_manager::RAISE:
        {
            const double total_pot = site_->get_total_pot();
            const double my_bet = site_->get_bet(0);
            const double op_bet = site_->get_bet(1);
            const double to_call = op_bet - my_bet;
            const double amount = raise_fraction * (total_pot + to_call) + to_call + my_bet;
            const double minbet = std::max(site_->get_big_blind(), to_call) + to_call + my_bet;

            BOOST_LOG_TRIVIAL(info) << QString("Raising %1 (%2x pot) (%3 min)").arg(amount).arg(raise_fraction)
                .arg(minbet).toStdString();

            site_->raise(amount, raise_fraction, minbet, action_delay_[1]);
        }
        break;
    }

    const double wait = site_->is_opponent_sitout() ? action_delay_[0] : get_normal_random(engine_,
        action_delay_[0], action_delay_[1]);

    BOOST_LOG_TRIVIAL(info) << QString("Waiting for %1 seconds after action").arg(wait).toStdString();

    next_action_times_[window.get_id()] = QDateTime::currentDateTime().addMSecs(static_cast<qint64>(wait * 1000));
}

void main_window::handle_lobby()
{
    if (!lobby_)
        return;

    if (!autolobby_action_->isChecked())
        return;

    if (schedule_action_->isChecked())
    {
        const auto active = is_schedule_active(activity_variance_, activity_spans_, &time_to_next_activity_);

        if (active != schedule_active_)
        {
            schedule_active_ = active;

            if (schedule_active_)
                BOOST_LOG_TRIVIAL(info) << "Enabling scheduled registration";
            else
                BOOST_LOG_TRIVIAL(info) << "Disabling scheduled registration";

            BOOST_LOG_TRIVIAL(info) << QString("Next scheduled %1 in %2").arg(schedule_active_ ? "break" : "activity")
                .arg(secs_to_hms(time_to_next_activity_)).toStdString();
        }
    }

    update_statusbar();

    assert(lobby_);

    const auto table_count_ok = lobby_->detect_closed_tables();

    if (table_count_ok && lobby_->get_registered_sngs() < table_count_->value()
        && (!schedule_action_->isChecked() || schedule_active_))
    {
        lobby_->register_sng();
    }
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

void main_window::update_strategy_widget(const fake_window& window, const nlhe_strategy& strategy, const std::array<int, 2>& hole,
    const std::array<int, 5>& board)
{
    const auto state = states_[window.get_id()];

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
    site_label_->setText(lobby_ ? QFileInfo(lobby_->get_filename().c_str()).fileName() : "No site");
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
        .arg(table_count_->value()).arg(lobby_ ? lobby_->get_active_tables() : 0));
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
