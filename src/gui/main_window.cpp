#include "main_window.h"

#pragma warning(push, 1)
#include <array>
#include <fstream>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/signals2.hpp>
#include <regex>
#include <boost/timer/timer.hpp>
#include <boost/algorithm/clamp.hpp>
#define NOMINMAX
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
#include "cfrlib/nl_holdem_state.h"
#include "cfrlib/strategy.h"
#include "table_widget.h"
#include "window_manager.h"
#include "holdem_strategy_widget.h"
#include "table_manager.h"
#include "input_manager.h"
#include "util/card.h"
#include "lobby_manager.h"
#include "state_widget.h"

#define INTERNAL_VERIFY(x) verify(x, #x, __LINE__)

namespace
{
    template<class T>
    typename T::const_iterator find_nearest(const T& map, const typename T::key_type& value)
    {
        auto upper = map.lower_bound(value);

        if (upper == map.begin() || upper->first == value)
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
}

main_window::main_window()
    : window_manager_(new window_manager)
    , engine_(std::random_device()())
    , play_(false)
    , input_manager_(new input_manager)
    , acting_(false)
    , logfile_(QDateTime::currentDateTimeUtc().toString("'log-'yyyyMMddTHHmmss'.txt'").toUtf8().data())
{
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
    title_filter_->setPlaceholderText("Table title");
    toolbar->addWidget(title_filter_);
    toolbar->addSeparator();
    lobby_title_ = new QLineEdit(this);
    lobby_title_->setPlaceholderText("Lobby title");
    connect(lobby_title_, SIGNAL(textChanged(const QString&)), SLOT(lobby_title_changed(const QString&)));
    toolbar->addWidget(lobby_title_);
    toolbar->addSeparator();
    table_count_ = new QSpinBox(this);
    table_count_->setValue(1);
    table_count_->setRange(1, 100);
    table_count_->setEnabled(false);
    toolbar->addWidget(table_count_);
    toolbar->addSeparator();
    capture_action_ = toolbar->addAction(QIcon(":/icons/control_record.png"), "Capture");
    capture_action_->setCheckable(true);
    connect(capture_action_, SIGNAL(toggled(bool)), SLOT(capture_changed(bool)));
    play_action_ = toolbar->addAction(QIcon(":/icons/control_play.png"), "Play");
    play_action_->setCheckable(true);
    play_action_->setEnabled(false);
    connect(play_action_, SIGNAL(toggled(bool)), SLOT(play_changed(bool)));

    log_ = new QPlainTextEdit(this);
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(1000);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(visualizer_);
    layout->addWidget(log_);
    widget->setLayout(layout);

    timer_ = new QTimer(this);
    connect(timer_, SIGNAL(timeout()), SLOT(timer_timeout()));
    play_timer_ = new QTimer(this);
    connect(play_timer_, SIGNAL(timeout()), SLOT(play_timer_timeout()));
    lobby_timer_ = new QTimer(this);
    connect(lobby_timer_, SIGNAL(timeout()), SLOT(lobby_timer_timeout()));

    strategy_label_ = new QLabel("No strategy", this);
    statusBar()->addWidget(strategy_label_, 1);
    capture_label_ = new QLabel("No window", this);
    statusBar()->addWidget(capture_label_, 1);

    QSettings settings("settings.ini", QSettings::IniFormat);
    capture_interval_ = settings.value("capture_interval", 0.1).toDouble();
    action_min_delay_ = settings.value("action_min_delay", 0.1).toDouble();
    action_max_delay_ = settings.value("action_max_delay", 15.0).toDouble();
    action_delay_mean_ = settings.value("action_delay_mean", 5).toDouble();
    action_delay_stddev_ = settings.value("action_delay_stddev", 2).toDouble();
    action_post_delay_ = settings.value("action_post_delay", 1.0).toDouble();
    input_manager_->set_delay_mean(settings.value("input_delay_mean", 0.1).toDouble());
    input_manager_->set_delay_stddev(settings.value("input_delay_stddev", 0.01).toDouble());
    lobby_interval_ = settings.value("lobby_interval", 0.1).toDouble();
    hotkey_ = settings.value("hotkey", VK_F1).toInt();

    log(QString("Loaded settings from \"%1\"").arg(settings.fileName()));

    while (!RegisterHotKey(winId(), 0, MOD_ALT | MOD_CONTROL, hotkey_))
        ++hotkey_;

    log(QString("Registered hot key Ctrl+Alt+%1").arg(get_key_text(hotkey_)));
}

main_window::~main_window()
{
}

void main_window::timer_timeout()
{
    if (window_manager_->is_stop())
    {
        capture_action_->setChecked(false);
        return;
    }

    find_window();
    process_snapshot();
}

void main_window::open_strategy()
{
    const auto filenames = QFileDialog::getOpenFileNames(this, "Open", QString(), "All files (*.*);;Site setting files (*.xml);;Strategy files (*.str)");

    if (filenames.empty())
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    strategy_infos_.clear();
    abstractions_.clear();

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

        const std::string filename = QFileInfo(*i).fileName().toUtf8().data();

        if (!std::regex_match(filename, m, r))
            continue;

        if (!std::regex_match(m[1].first, m[1].second, m_nlhe, r_nlhe))
            continue;

        const std::string actions = m_nlhe[1];
        const auto stack_size = boost::lexical_cast<int>(m_nlhe[2]);
        auto& si = strategy_infos_[stack_size];
        si.reset(new strategy_info);

        if (actions == "fchpa")
            si->root_state_.reset(new nl_holdem_state<F_MASK | C_MASK | H_MASK | P_MASK | A_MASK>(stack_size));
        else if (actions == "fchqpa")
            si->root_state_.reset(new nl_holdem_state<F_MASK | C_MASK | H_MASK | Q_MASK | P_MASK | A_MASK>(stack_size));

        std::size_t states = 0;
        std::vector<const nlhe_state_base*> stack(1, si->root_state_.get());

        while (!stack.empty())
        {
            const nlhe_state_base* s = stack.back();
            stack.pop_back();

            if (!s->is_terminal())
                ++states;

            for (int i = s->get_action_count() - 1; i >= 0; --i)
            {
                if (const nlhe_state_base* child = s->get_child(i))
                    stack.push_back(child);
            }
        }

        const auto dir = QFileInfo(*i).dir();
        const std::string abs_filename = dir.filePath(QString(m[2].str().data()) + ".abs").toUtf8().data();
        si->abstraction_ = abs_filename;

        if (!abstractions_.count(abs_filename))
            abstractions_[abs_filename].reset(new holdem_abstraction(abs_filename));

        const std::string str_filename = i->toUtf8().data();
        si->strategy_.reset(new strategy(str_filename, states, si->root_state_->get_action_count()));

        log(QString("Loaded strategy \"%1\"").arg(*i));
    }

    QApplication::restoreOverrideCursor();
}

void main_window::capture_changed(const bool checked)
{
    if (checked)
    {
        timer_->start(int(capture_interval_ * 1000.0));
        window_manager_->set_title_filter(std::string(title_filter_->text().toUtf8().data()));

        for (auto it = strategy_infos_.begin(); it != strategy_infos_.end(); ++it)
            it->second->current_state_ = nullptr;
    }
    else
    {
        timer_->stop();
        window_manager_->clear_window();
        play_action_->setChecked(false);
        site_->reset();
        lobby_->reset();
    }

    title_filter_->setEnabled(!checked);
    lobby_title_->setEnabled(!checked);
    table_count_->setEnabled(!checked);
    play_action_->setEnabled(checked);
    open_action_->setEnabled(!checked);

}

void main_window::show_strategy_changed()
{
    strategy_->setVisible(!strategy_->isVisible());
}

void main_window::play_changed(const bool checked)
{
    play_ = checked;

    if (play_)
    {
        capture_action_->setChecked(true);
        lobby_timer_->start(int(lobby_interval_ * 1000.0));
    }

    if (!play_)
    {
        play_timer_->stop();
        lobby_timer_->stop();
    }
}

void main_window::play_timer_timeout()
{
    assert(play_);
    log("Player: Waiting for mutex...");

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
            const double big_blind = site_->get_big_blind();
            const double to_call = op_bet - my_bet;
            const double amount = int((raise_fraction_ * (total_pot + to_call) + to_call + my_bet) / big_blind) * big_blind;

            log(QString("Player: Raise %1 (%2x pot)").arg(amount).arg(raise_fraction_));
            site_->raise(amount, raise_fraction_);
        }
        break;
    }

    log(QString("Player: Waiting for %1 seconds after action...").arg(action_post_delay_));
    QTimer::singleShot(int(action_post_delay_ * 1000.0), this, SLOT(play_done_timeout()));
}

void main_window::find_window()
{
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

    if (site_)
        site_->set_window(window);
}

void main_window::process_snapshot()
{
    if (acting_ || !site_ || !site_->is_window())
        return;

    boost::timer::cpu_timer t;

    site_->update(save_images_->isChecked());

    t.stop();

    const auto hole = site_->get_hole_cards();
    std::array<int, 5> board;
    site_->get_board_cards(board);

    int round;

    if (board[4] != -1)
        round = holdem_abstraction::RIVER;
    else if (board[3] != -1)
        round = holdem_abstraction::TURN;
    else if (board[0] != -1)
        round = holdem_abstraction::FLOP;
    else if (hole.first != -1 && hole.second != -1)
        round = holdem_abstraction::PREFLOP;
    else
        round = -1;

    const bool new_game = round == 0
        && ((site_->get_dealer() == 0 && site_->get_bet(0) < site_->get_big_blind())
        || (site_->get_dealer() == 1 && site_->get_bet(0) == site_->get_big_blind()));

#if !defined(NDEBUG)
    log(QString("new_game:%1 round:%2 bet0:%3 bet1:%4 stack0:%5 stack1:%6 allin:%7 sitout:%8 pot:%9 buttons:%10")
        .arg(new_game)
        .arg(round)
        .arg(site_->get_bet(0))
        .arg(site_->get_bet(1))
        .arg(site_->get_stack(0))
        .arg(site_->get_stack(1))
        .arg(site_->is_opponent_allin())
        .arg(site_->is_opponent_sitout())
        .arg(site_->get_total_pot())
        .arg(site_->get_buttons()));
#endif

    visualizer_->set_dealer(site_->get_dealer());
    std::array<int, 2> hole_array = {{ hole.first, hole.second }};
    visualizer_->set_hole_cards(0, hole_array);
    visualizer_->set_board_cards(board);

    if (site_->is_sit_out(0))
    {
        log("Warning: We are sitting out");

        if (play_)
        {
            log("Player: Sitting in");
            auto mutex = window_manager_->try_interact();
            site_->sit_in();
        }
    }

    // wait until we see buttons
    if (site_->get_buttons() == 0)
        return;

    // wait until we see stack sizes
    if (site_->get_stack(0) == 0 || (site_->get_stack(1) == 0 && !site_->is_opponent_allin() && !site_->is_opponent_sitout()))
        return;

    snapshot_.round = round;
    snapshot_.bet = site_->get_bet(1);

    // our stack size should always be visible
    INTERNAL_VERIFY(site_->get_stack(0) > 0);
    // opponent stack might be obstructed by all-in or sitout statuses
    INTERNAL_VERIFY(site_->get_stack(1) > 0 || site_->is_opponent_allin() || site_->is_opponent_sitout());

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
                stacks[1] = 9999; // can't see stack due to sitout, assume worst case
            else if (site_->is_opponent_allin())
                stacks[1] = site_->get_bet(1); // opponent stack equals his bet
        }

        INTERNAL_VERIFY(stacks[0] > 0 && stacks[1] > 0);
        snapshot_.stack_size = int(std::min(stacks[0], stacks[1]) / site_->get_big_blind() * 2 + 0.5);
    }

    log(QString("*** SNAPSHOT (%1 ms) ***").arg(t.elapsed().wall / 1000000.0));

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
        current_state = strategy_info.root_state_.get();
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

    if ((current_state->get_round() == holdem_abstraction::PREFLOP && (site_->get_bet(1) > site_->get_big_blind()))
        || (current_state->get_round() > holdem_abstraction::PREFLOP && (site_->get_bet(1) > 0)))
    {
        // make sure opponent allin is always terminal on his part and doesnt get translated to something else
        const double fraction = site_->is_opponent_allin() ? 999.0 : (site_->get_bet(1) - site_->get_bet(0))
            / (site_->get_total_pot() - (site_->get_bet(1) - site_->get_bet(0)));
        INTERNAL_VERIFY(fraction > 0);
        log(QString("State: Opponent raised %1x pot").arg(fraction));
        current_state = current_state->raise(fraction); // there is an outstanding bet/raise
    }
    else if ((current_state->get_round() == holdem_abstraction::PREFLOP && site_->get_dealer() == 1 && site_->get_bet(1) <= site_->get_big_blind())
        || (current_state->get_round() > holdem_abstraction::PREFLOP && site_->get_dealer() == 0 && site_->get_bet(1) == 0))
    {
        // we are in position facing 0 sized bet, opponent has checked
        // we are oop facing big blind sized bet preflop, opponent has called
        log("State: Opponent called");
        current_state = current_state->call();
    }

    INTERNAL_VERIFY(current_state != nullptr);

    // ensure it is our turn
    INTERNAL_VERIFY((site_->get_dealer() == 0 && current_state->get_player() == 0)
            || (site_->get_dealer() == 1 && current_state->get_player() == 1));

    // ensure rounds match
    assert(current_state->get_round() == round);

    std::array<int, 2> pot = current_state->get_pot();
        
    if (site_->get_dealer() == 1)
        std::swap(pot[0], pot[1]);

    visualizer_->set_pot(current_state->get_round(), pot);

    const auto& strategy = strategy_info.strategy_;
    strategy_label_->setText(QString("%1").arg(QFileInfo(strategy->get_filename().c_str()).fileName()));

    // we should never reach terminal states when we have a pending action
    assert(current_state->get_id() != -1);

    std::stringstream ss;
    ss << *current_state;
    log(QString("State: %1").arg(ss.str().c_str()));

    update_strategy_widget(strategy_info);

    perform_action();
}

void main_window::perform_action()
{
    assert(site_ && !strategy_infos_.empty() && !play_timer_->isActive());

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
    const auto abstraction = *abstractions_.at(strategy_info.abstraction_);

    if (c0 != -1 && c1 != -1)
    {
        if (b4 != -1)
            bucket = abstraction.get_bucket(c0, c1, b0, b1, b2, b3, b4);
        else if (b3 != -1)
            bucket = abstraction.get_bucket(c0, c1, b0, b1, b2, b3);
        else if (b0 != -1)
            bucket = abstraction.get_bucket(c0, c1, b0, b1, b2);
        else
            bucket = abstraction.get_bucket(c0, c1);
    }

    auto& current_state = strategy_info.current_state_;

    assert(current_state && !current_state->is_terminal() && bucket != -1);

    const auto& strategy = strategy_info.strategy_;
    const int index = site_->is_opponent_sitout() ? nlhe_state_base::CALL + 1
        : strategy->get_action(current_state->get_id(), bucket);
    const int action = current_state->get_action(index);
    std::string s = "n/a";

    switch (action)
    {
    case nlhe_state_base::FOLD:
        next_action_ = table_manager::FOLD;
        s = "FOLD";
        break;
    case nlhe_state_base::CALL:
        {
            const int prev_action_index = current_state->get_action();
            const int prev_action = prev_action_index != -1 ? current_state->get_action(prev_action_index) : -1;

            // ensure we get allin if the opponent went allin
            if (prev_action == nlhe_state_base::RAISE_A)
            {
                next_action_ = table_manager::RAISE;
                raise_fraction_ = 999.0;
            }
            else
            {
                next_action_ = table_manager::CALL;
            }

            s = "CALL";
        }
        break;
    case nlhe_state_base::RAISE_H:
        next_action_ = table_manager::RAISE;
        raise_fraction_ = 0.5;
        s = "RAISE_H";
        break;
    case nlhe_state_base::RAISE_P:
        next_action_ = table_manager::RAISE;
        raise_fraction_ = 1.0;
        s = "RAISE_P";
        break;
    case nlhe_state_base::RAISE_A:
        next_action_ = table_manager::RAISE;
        raise_fraction_ = 999.0;
        s = "RAISE_A";
        break;
    default:
        assert(false);
    }

    const double probability = strategy->get(current_state->get_id(), index, bucket);
    log(QString("Strategy: %1 (%2)").arg(s.c_str()).arg(probability));

    current_state = current_state->get_child(index);

    acting_ = true;

    if (play_)
    {
        std::normal_distribution<> dist(action_delay_mean_, action_delay_stddev_);
        const double wait = site_->is_opponent_sitout() ? action_min_delay_ : boost::algorithm::clamp(dist(engine_),
            action_min_delay_, action_max_delay_);
        play_timer_->setSingleShot(true);
        play_timer_->start(int(wait * 1000.0));
        log(QString("Player: Waiting %1 seconds...").arg(wait));
    }
    else
    {
        log(QString("Player: Waiting for %1 seconds to continue...").arg(action_max_delay_));
        QTimer::singleShot(int(action_max_delay_ * 1000.0), this, SLOT(play_done_timeout()));
    }
}

main_window::strategy_info::strategy_info()
    : current_state_(nullptr)
{
}

main_window::strategy_info::~strategy_info()
{
}

void main_window::play_done_timeout()
{
    acting_ = false;
}

void main_window::lobby_timer_timeout()
{
    if (!lobby_)
        return;

    if (!lobby_->is_window())
    {
        const auto filter = lobby_title_->text();

        if (filter.isEmpty())
            return;

        const auto window = FindWindow(nullptr, filter.toUtf8().data());

        if (!IsWindow(window))
            return;

        lobby_->set_window(window);
    }

    assert(lobby_ && lobby_->is_window());

    auto mutex = window_manager_->try_interact();

    //log(QString("Registered in %1 tournaments").arg(lobby_->get_registered_sngs()));

    if (lobby_->get_registered_sngs() < table_count_->value())
        lobby_->register_sng();

    const auto old = lobby_->get_registered_sngs();
    lobby_->close_popups();

    if (lobby_->get_registered_sngs() != old)
        log(QString("Lobby: Registration count changed (%1 -> %2)").arg(old).arg(lobby_->get_registered_sngs()));
}

void main_window::log(const QString& s)
{
    const auto message = QString("[%1] %2").arg(QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss")).arg(s);
    log_->appendPlainText(message);
    logfile_ << message.toUtf8().data() << std::endl;
}

bool main_window::winEvent(MSG* message, long*)
{
    if (message->message == WM_HOTKEY)
    {
        play_action_->trigger();
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
    std::string s(board.toUtf8().data());
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
    case holdem_game::PREFLOP:
        board[0] = -1;
        board[1] = -1;
        board[2] = -1;
    case holdem_game::FLOP:
        board[3] = -1;
    case holdem_game::TURN:
        board[4] = -1;
    }

    strategy_->update(*abstractions_.at(si.abstraction_), hole, board, *si.strategy_, si.current_state_->get_id(),
        si.current_state_->get_action_count());

    std::stringstream ss;
    ss << *si.current_state_;

    state_widget_->set_state(ss.str().c_str());
}

void main_window::state_widget_state_reset()
{
    if (strategy_infos_.empty())
        return;

    auto& si = *strategy_infos_.begin()->second;
    si.current_state_ = si.root_state_.get();
    update_strategy_widget(*strategy_infos_.begin()->second);
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
    update_strategy_widget(*strategy_infos_.begin()->second);
}

void main_window::state_widget_raised(double fraction)
{
    if (strategy_infos_.empty())
        return;

    auto& si = *strategy_infos_.begin()->second;

    if (!si.current_state_)
        return;
 
    si.current_state_ = si.current_state_->raise(fraction);
    update_strategy_widget(*strategy_infos_.begin()->second);
}

void main_window::verify(bool expression, const std::string& s, int line)
{
    if (expression)
        return;

    assert(false);
    log(QString("Error: verification on line %1 failed (%2)").arg(line).arg(s.c_str()));
    window_manager_->stop();
}

void main_window::lobby_title_changed(const QString& str)
{
    table_count_->setEnabled(!str.isEmpty());
}
