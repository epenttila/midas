#include "main_window.h"

#pragma warning(push, 1)
#include <array>
#include <fstream>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/signals2.hpp>
#include <regex>
#include <boost/timer/timer.hpp>
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
#pragma warning(pop)

#include "site_stars.h"
#include "cfrlib/holdem_game.h"
#include "cfrlib/nl_holdem_state.h"
#include "cfrlib/strategy.h"
#include "table_widget.h"
#include "window_manager.h"
#include "holdem_strategy_widget.h"
#include "site_888.h"
#include "site_base.h"
#include "settings_dialog.h"
#include "input_manager.h"
#include "util/card.h"

namespace
{
    enum { SITE_STARS, SITE_888 };

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
}

main_window::main_window()
    : window_manager_(new window_manager)
    , engine_(std::random_device()())
    , play_(false)
    , input_manager_(new input_manager)
    , acting_(false)
{
    auto widget = new QWidget(this);
    widget->setFocus();

    setCentralWidget(widget);

    visualizer_ = new table_widget(this);
    strategy_ = new holdem_strategy_widget(this, Qt::Tool);
    strategy_->setVisible(false);

    auto toolbar = addToolBar("File");
    toolbar->setMovable(false);
    auto action = toolbar->addAction(QIcon(":/icons/folder_page_white.png"), "&Open strategy...");
    action->setIconText("Open strategy...");
    action->setToolTip("Open strategy...");
    connect(action, SIGNAL(triggered()), SLOT(open_strategy()));
    action = toolbar->addAction(QIcon(":/icons/table.png"), "Show strategy");
    action->setCheckable(true);
    connect(action, SIGNAL(changed()), SLOT(show_strategy_changed()));
    action = toolbar->addAction(QIcon(":/icons/cog.png"), "Settings");
    connect(action, SIGNAL(triggered()), SLOT(settings_triggered()));
    toolbar->addSeparator();

    site_list_ = new QComboBox(this);
    site_list_->insertItem(SITE_STARS, "PokerStars", SITE_STARS);
    site_list_->insertItem(SITE_888, "888 Poker", SITE_888);
    action = toolbar->addWidget(site_list_);

    toolbar->addSeparator();
    title_filter_ = new QLineEdit(this);
    title_filter_->setPlaceholderText("Window title");
    toolbar->addWidget(title_filter_);
    toolbar->addSeparator();
    action = toolbar->addAction(QIcon(":/icons/control_record.png"), "Capture");
    action->setCheckable(true);
    connect(action, SIGNAL(changed()), SLOT(capture_changed()));
    action = toolbar->addAction(QIcon(":/icons/control_play.png"), "Play");
    action->setCheckable(true);
    connect(action, SIGNAL(changed()), SLOT(play_changed()));
    step_action_ = toolbar->addAction(QIcon(":/icons/control_step.png"), "Step");
    step_action_->setEnabled(false);
    connect(action, SIGNAL(triggered()), SLOT(step_triggered()));

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

    strategy_label_ = new QLabel("No strategy", this);
    statusBar()->addWidget(strategy_label_, 1);
    capture_label_ = new QLabel("No window", this);
    statusBar()->addWidget(capture_label_, 1);

    QCoreApplication::setOrganizationName("baabeli.com");
    QCoreApplication::setApplicationName("midas");

    QSettings settings;
    capture_interval_ = settings.value("capture_interval", 0.1).toDouble();
    action_min_delay_ = settings.value("action_min_delay", 0.1).toDouble();
    action_delay_mean_ = settings.value("action_delay_mean", 5).toDouble();
    action_delay_stddev_ = settings.value("action_delay_dev", 2).toDouble();
    input_manager_->set_delay_mean(settings.value("input_delay_mean", 0.1).toDouble());
    input_manager_->set_delay_stddev(settings.value("input_delay_stddev", 0.01).toDouble());
}

main_window::~main_window()
{
}

void main_window::timer_timeout()
{
    find_window();
    process_snapshot();
}

void main_window::open_strategy()
{
    const auto filenames = QFileDialog::getOpenFileNames(this, "Open Strategy", QString(), "Strategy files (*.str)");

    QApplication::setOverrideCursor(Qt::WaitCursor);

    strategy_infos_.clear();

    std::regex r("([^-]+)-([^-]+)-[0-9]+\\.str");
    std::regex r_nlhe("nlhe\\.([a-z]+)\\.([0-9]+)");
    std::smatch m;
    std::smatch m_nlhe;

    for (auto i = filenames.begin(); i != filenames.end(); ++i)
    {
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
        si->abstraction_.reset(new holdem_abstraction(abs_filename));

        const std::string str_filename = i->toUtf8().data();
        si->strategy_.reset(new strategy(str_filename, states, si->root_state_->get_action_count()));
    }

    QApplication::restoreOverrideCursor();

    log_->appendPlainText(QString("%1 strategy files loaded").arg(filenames.size()));
}

void main_window::capture_changed()
{
    if (!timer_->isActive())
    {
        timer_->start(int(capture_interval_ * 1000.0));
        site_list_->setEnabled(false);
        title_filter_->setEnabled(false);
        window_manager_->set_title_filter(std::string(title_filter_->text().toUtf8().data()));
    }
    else
    {
        timer_->stop();
        site_list_->setEnabled(true);
        title_filter_->setEnabled(true);
        window_manager_->clear_window();
    }
}

void main_window::show_strategy_changed()
{
    strategy_->setVisible(!strategy_->isVisible());
}

void main_window::play_changed()
{
    play_ = !play_;

    if (!play_)
        play_timer_->stop();
}

void main_window::play_timer_timeout()
{
    assert(play_);
    log_->appendPlainText("Player: Waiting for mutex...");

    auto mutex = window_manager_->try_interact();

    switch (next_action_)
    {
    case site_base::FOLD:
        log_->appendPlainText("Player: Fold");
        site_->fold();
        break;
    case site_base::CALL:
        log_->appendPlainText("Player: Call");
        site_->call();
        break;
    case site_base::RAISE:
        {
            const double total_pot = site_->get_total_pot();
            const double my_bet = site_->get_bet(0);
            const double op_bet = site_->get_bet(1);
            const double big_blind = site_->get_big_blind();
            const double to_call = op_bet - my_bet;
            const double amount = int((raise_fraction_ * (total_pot + to_call) + to_call + my_bet) / big_blind) * big_blind;

            log_->appendPlainText(QString("Player: Raise %1").arg(amount));
            site_->raise(amount);
        }
        break;
    }

    acting_ = false;
    step_action_->setEnabled(acting_);
}

void main_window::find_window()
{
    if (window_manager_->is_window())
        return;

    if (window_manager_->find_window())
    {
        const auto window = window_manager_->get_window();

        switch (site_list_->itemData(site_list_->currentIndex()).toInt())
        {
        case SITE_STARS:
            site_.reset(new site_stars(window));
            break;
        case SITE_888:
            site_.reset(new site_888(*input_manager_, window));
            break;
        default:
            assert(false);
        }

        const std::string title_name = window_manager_->get_title_name();
        capture_label_->setText(QString("%1").arg(title_name.c_str()));

        snapshot_.round = -1;
        snapshot_.bet = -1;
    }
    else
    {
        site_.reset();
        capture_label_->setText("No window");
    }
}

void main_window::process_snapshot()
{
    if (acting_ || !site_)
        return;

    boost::timer::cpu_timer t;

    site_->update();

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
    log_->appendPlainText(QString("new_game:%1 round:%2 bet0:%3 bet1:%4 stack0:%5 stack1:%6 allin:%7 sitout:%8")
        .arg(new_game)
        .arg(round)
        .arg(site_->get_bet(0))
        .arg(site_->get_bet(1))
        .arg(site_->get_stack(0))
        .arg(site_->get_stack(1))
        .arg(site_->is_opponent_allin())
        .arg(site_->is_opponent_sitout()));
#endif

    visualizer_->set_dealer(site_->get_dealer());
    std::array<int, 2> hole_array = {{ hole.first, hole.second }};
    visualizer_->set_hole_cards(0, hole_array);
    visualizer_->set_board_cards(board);

    // wait until we see buttons
    if (site_->get_buttons() == 0)
        return;

    // TODO allin states postflop are sometimes not detected correctly, remove new_game check here?
    // wait until we see stack sizes at the start of a hand
    if (new_game
        && (site_->get_stack(0) == 0
        || (site_->get_stack(1) == 0 && !site_->is_opponent_allin() && !site_->is_opponent_sitout())))
    {
        return;
    }

    acting_ = true;
    step_action_->setEnabled(acting_);

    snapshot_.round = round;
    snapshot_.bet = site_->get_bet(1);

    // our stack size should always be visible
    assert(site_->get_stack(0) > 0);
    // opponent stack might be obstructed by all-in or sitout statuses
    assert(site_->get_stack(1) > 0 || site_->is_opponent_allin() || site_->is_opponent_sitout());

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

        assert(stacks[0] > 0 && stacks[1] > 0);
        snapshot_.stack_size = int(std::min(stacks[0], stacks[1]) / site_->get_big_blind() * 2 + 0.5);
    }

    log_->appendPlainText(QString("*** SNAPSHOT (%1 ms) ***").arg(t.elapsed().wall / 1000000.0));

    if (hole.first != -1 && hole.second != -1)
    {
        log_->appendPlainText(QString("Hole: [%1 %2]").arg(get_card_string(hole.first).c_str())
           .arg(get_card_string(hole.second).c_str()));
    }

    if (board[4] != -1)
    {
        log_->appendPlainText(QString("Board: [%1 %2 %3 %4] [%5]").arg(get_card_string(board[0]).c_str()).arg(get_card_string(board[1]).c_str())
            .arg(get_card_string(board[2]).c_str()).arg(get_card_string(board[3]).c_str()).arg(get_card_string(board[4]).c_str()));
    }
    else if (board[3] != -1)
    {
        log_->appendPlainText(QString("Board: [%1 %2 %3] [%4]").arg(get_card_string(board[0]).c_str()).arg(get_card_string(board[1]).c_str())
            .arg(get_card_string(board[2]).c_str()).arg(get_card_string(board[3]).c_str()));
    }
    else if (board[0] != -1)
    {
        log_->appendPlainText(QString("Board: [%1 %2 %3]").arg(get_card_string(board[0]).c_str()).arg(get_card_string(board[1]).c_str())
            .arg(get_card_string(board[2]).c_str()));
    }

    if (strategy_infos_.empty())
        return;

    auto it = find_nearest(strategy_infos_, snapshot_.stack_size);
    auto& strategy_info = *it->second;
    auto& current_state = strategy_info.current_state_;

    if (new_game && round == 0)
    {
        log_->appendPlainText(QString("State: New game (%1 SB)").arg(snapshot_.stack_size));
        current_state = strategy_info.root_state_.get();
    }

    if (!current_state)
        return;

    // a new round has started but we did not end the previous one, opponent has called
    if (current_state->get_round() != round)
    {
        log_->appendPlainText("State: Opponent called");
        current_state = current_state->call();
    }

    if (site_->is_opponent_sitout())
    {
        log_->appendPlainText("State: Opponent is sitting out");
    }
    else if (site_->is_opponent_allin())
    {
        log_->appendPlainText("State: Opponent is all-in");
        current_state = current_state->raise(999.0);
    }
    else if ((current_state->get_round() == holdem_abstraction::PREFLOP && (site_->get_bet(1) > site_->get_big_blind()))
        || (current_state->get_round() > holdem_abstraction::PREFLOP && (site_->get_bet(1) > 0)))
    {
        const double fraction = (site_->get_bet(1) - site_->get_bet(0)) / site_->get_total_pot();
        assert(fraction > 0);
        log_->appendPlainText(QString("State: Opponent raised %1").arg(fraction));
        current_state = current_state->raise(fraction); // there is an outstanding bet/raise
    }
    else if ((current_state->get_round() > holdem_abstraction::PREFLOP && site_->get_dealer() == 0 && site_->get_bet(1) == 0)
        || (current_state->get_round() == holdem_abstraction::PREFLOP && site_->get_dealer() == 1 && site_->get_bet(1) == site_->get_big_blind()))
    {
        // we are in position facing 0 sized bet, opponent has checked
        // we are oop facing big blind sized bet preflop, opponent has called
        log_->appendPlainText("State: Opponent called");
        current_state = current_state->call();
    }

    assert(current_state);

    // ensure it is our turn
    assert((site_->get_dealer() == 0 && current_state->get_player() == 0)
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
    log_->appendPlainText(QString("State: %1").arg(ss.str().c_str()));
    strategy_->update(*strategy_info.abstraction_, board, *strategy, current_state->get_id(),
        current_state->get_action_count());

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

    if (c0 != -1 && c1 != -1)
    {
        if (b4 != -1)
            bucket = strategy_info.abstraction_->get_bucket(c0, c1, b0, b1, b2, b3, b4);
        else if (b3 != -1)
            bucket = strategy_info.abstraction_->get_bucket(c0, c1, b0, b1, b2, b3);
        else if (b0 != -1)
            bucket = strategy_info.abstraction_->get_bucket(c0, c1, b0, b1, b2);
        else
            bucket = strategy_info.abstraction_->get_bucket(c0, c1);
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
        next_action_ = site_base::FOLD;
        s = "FOLD";
        break;
    case nlhe_state_base::CALL:
        {
            const int prev_action_index = current_state->get_action();
            const int prev_action = prev_action_index != -1 ? current_state->get_action(prev_action_index) : -1;

            // ensure we get allin if the opponent went allin
            if (prev_action == nlhe_state_base::RAISE_A)
            {
                next_action_ = site_base::RAISE;
                raise_fraction_ = 999.0;
            }
            else
            {
                next_action_ = site_base::CALL;
            }

            s = "CALL";
        }
        break;
    case nlhe_state_base::RAISE_H:
        next_action_ = site_base::RAISE;
        raise_fraction_ = 0.5;
        s = "RAISE_H";
        break;
    case nlhe_state_base::RAISE_P:
        next_action_ = site_base::RAISE;
        raise_fraction_ = 1.0;
        s = "RAISE_P";
        break;
    case nlhe_state_base::RAISE_A:
        next_action_ = site_base::RAISE;
        raise_fraction_ = 999.0;
        s = "RAISE_A";
        break;
    default:
        assert(false);
    }

    const double probability = strategy->get(current_state->get_id(), index, bucket);
    log_->appendPlainText(QString("Strategy: %1 (%2)").arg(s.c_str()).arg(probability));

    current_state = current_state->get_child(index);

    if (play_)
    {
        std::normal_distribution<> dist(action_delay_mean_, action_delay_stddev_);
        const double wait = site_->is_opponent_sitout() ? action_min_delay_ : std::max(action_min_delay_, dist(engine_));
        play_timer_->setSingleShot(true);
        play_timer_->start(int(wait * 1000.0));
        log_->appendPlainText(QString("Player: Waiting %1 seconds...").arg(wait));
    }
}

void main_window::settings_triggered()
{
    settings_dialog d(capture_interval_, action_min_delay_, action_delay_mean_, action_delay_stddev_,
        input_manager_->get_delay_mean(), input_manager_->get_delay_stddev(), this);
    
    if (d.exec() == QDialog::Accepted)
    {
        capture_interval_ = d.get_capture_interval();
        action_min_delay_ = d.get_action_min_delay();
        action_delay_mean_ = d.get_action_delay_mean();
        action_delay_stddev_ = d.get_action_delay_stddev();
        input_manager_->set_delay_mean(d.get_input_delay_mean());
        input_manager_->set_delay_stddev(d.get_input_delay_stddev());
    }
}

void main_window::closeEvent(QCloseEvent* event)
{
    QSettings settings;
    settings.setValue("capture_interval", capture_interval_);
    settings.setValue("action_min_delay", action_min_delay_);
    settings.setValue("action_delay_mean", action_delay_mean_);
    settings.setValue("action_delay_dev", action_delay_stddev_);
    settings.setValue("input_delay_mean", input_manager_->get_delay_mean());
    settings.setValue("input_delay_stddev", input_manager_->get_delay_stddev());
    event->accept();
}

void main_window::step_triggered()
{
    // TODO this doesnt work for some reason
    acting_ = false;
    step_action_->setEnabled(acting_);
}

main_window::strategy_info::strategy_info()
    : current_state_(nullptr)
{
}

main_window::strategy_info::~strategy_info()
{
}
