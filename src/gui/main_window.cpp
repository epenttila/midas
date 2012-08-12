#include "main_window.h"

#pragma warning(push, 1)
#include <array>
#include <fstream>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/signals2.hpp>
#include <regex>
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
{
    auto widget = new QFrame(this);
    widget->setFrameStyle(QFrame::StyledPanel);
    widget->setFocus();

    setCentralWidget(widget);

    visualizer_ = new table_widget(this);
    strategy_ = new holdem_strategy_widget(this);
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

    log_ = new QPlainTextEdit(this);
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(1000);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(visualizer_);
    layout->addWidget(log_);
    layout->addWidget(strategy_);
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

        si->current_state_ = si->root_state_.get();

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

    log_->appendPlainText(QString("%1 strategies loaded").arg(filenames.size()));
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
    log_->appendPlainText("Waiting for mutex...");

    auto mutex = window_manager_->try_interact();

    switch (next_action_)
    {
    case site_base::FOLD:
        log_->appendPlainText("Folding");
        site_->fold();
        break;
    case site_base::CALL:
        log_->appendPlainText("Calling");
        site_->call();
        break;
    case site_base::RAISE:
        log_->appendPlainText(QString("Raising %1").arg(raise_fraction_));
        site_->raise(raise_fraction_);
        break;
    }
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
    }
    else
    {
        site_.reset();
        capture_label_->setText("No window");
    }
}

void main_window::process_snapshot()
{
    if (!site_ || !site_->update())
        return;

    visualizer_->set_dealer(site_->get_dealer());

    auto hole = site_->get_hole_cards();
    std::array<int, 2> hole_array = {{ hole.first, hole.second }};
    visualizer_->set_hole_cards(0, hole_array);

    std::array<int, 5> board;
    site_->get_board_cards(board);
    visualizer_->set_board_cards(board);

    if (strategy_infos_.empty())
        return;

    auto stack_size = site_->get_stack_size();
    auto it = find_nearest(strategy_infos_, stack_size);
    auto& strategy_info = *it->second;
    auto& current_state = strategy_info.current_state_;

    if (site_->is_new_hand())
        current_state = strategy_info.root_state_.get();

    if (!current_state)
        return;

    switch (site_->get_action())
    {
    case site_base::CALL:
        current_state = current_state->call();
        break;
    case site_base::RAISE:
        current_state = current_state->raise(site_->get_raise_fraction());
        break;
    }

    if (!current_state)
        return;

    if (current_state->get_round() != site_->get_round())
        return;

    std::array<int, 2> pot = current_state->get_pot();
        
    if (site_->get_dealer() == 1)
        std::swap(pot[0], pot[1]);

    visualizer_->set_pot(current_state->get_round(), pot);

    const auto& strategy = strategy_info.strategy_;
    strategy_label_->setText(QString("%1").arg(QFileInfo(strategy->get_filename().c_str()).fileName()));

    if (current_state && current_state->get_id() != -1)
    {
        std::stringstream ss;
        ss << *current_state;
        log_->appendPlainText(ss.str().c_str());
        strategy_->update(*strategy_info.abstraction_, board, *strategy, current_state->get_id(),
            current_state->get_action_count());
    }

    perform_action();
}

void main_window::perform_action()
{
    if (!play_ || !site_ || strategy_infos_.empty() || !site_->is_action_needed() || play_timer_->isActive())
        return;

    auto stack_size = site_->get_stack_size();
    auto it = find_nearest(strategy_infos_, stack_size);
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

    const auto& current_state = strategy_info.current_state_;

    if (current_state && !current_state->is_terminal() && bucket != -1)
    {
        const auto& strategy = strategy_info.strategy_;
        const int index = strategy->get_action(current_state->get_id(), bucket);
        const int action = current_state->get_action(index);
        std::string s = "n/a";

        switch (action)
        {
        case nlhe_state_base::FOLD: s = "FOLD"; break;
        case nlhe_state_base::CALL: s = "CALL"; break;
        case nlhe_state_base::RAISE_H: s = "RAISE_H"; break;
        case nlhe_state_base::RAISE_Q: s = "RAISE_Q"; break;
        case nlhe_state_base::RAISE_P: s = "RAISE_P"; break;
        case nlhe_state_base::RAISE_A: s = "RAISE_A"; break;
        }

        double probability = strategy->get(current_state->get_id(), index, bucket);
        log_->appendPlainText(QString("%1 (%2%)").arg(s.c_str()).arg(int(probability * 100)));

        switch (action)
        {
        case nlhe_state_base::FOLD:
            next_action_ = site_base::FOLD;
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
            }
            break;
        case nlhe_state_base::RAISE_H:
            next_action_ = site_base::RAISE;
            raise_fraction_ = 0.5;
            break;
        case nlhe_state_base::RAISE_P:
            next_action_ = site_base::RAISE;
            raise_fraction_ = 1.0;
            break;
        case nlhe_state_base::RAISE_A:
            next_action_ = site_base::RAISE;
            raise_fraction_ = 999.0;
            break;
        }
    }
    else
    {
        log_->appendPlainText("Warning: unknown state, folding...");
        next_action_ = site_base::FOLD;
    }

    std::normal_distribution<> dist(action_delay_mean_, action_delay_stddev_);
    const double wait = std::max(capture_interval_ * 2.0, dist(engine_));
    play_timer_->setSingleShot(true);
    play_timer_->start(int(wait * 1000.0));
    log_->appendPlainText(QString("Waiting %1 seconds...").arg(wait));
}

void main_window::settings_triggered()
{
    settings_dialog d(capture_interval_, action_delay_mean_, action_delay_stddev_, input_manager_->get_delay_mean(),
        input_manager_->get_delay_stddev(), this);
    
    if (d.exec() == QDialog::Accepted)
    {
        capture_interval_ = d.get_capture_interval();
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
    settings.setValue("action_delay_mean", action_delay_mean_);
    settings.setValue("action_delay_dev", action_delay_stddev_);
    settings.setValue("input_delay_mean", input_manager_->get_delay_mean());
    settings.setValue("input_delay_stddev", input_manager_->get_delay_stddev());
    event->accept();
}

main_window::strategy_info::strategy_info()
    : current_state_(nullptr)
{
}

main_window::strategy_info::~strategy_info()
{
}
