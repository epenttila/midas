#include "main_window.h"

#pragma warning(push, 3)
#include <array>
#include <fstream>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/signals2.hpp>
#include <Windows.h>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QMenuBar>
#include <QFileDialog>
#include <QStatusBar>
#include <QLabel>
#pragma warning(pop)

#include "site_stars.h"
#include "cfrlib/holdem_game.h"
#include "cfrlib/nl_holdem_state.h"
#include "cfrlib/strategy.h"
#include "table_widget.h"
#include "strategy_dialog.h"

namespace
{
    class capture_button : public QPushButton
    {
    public:
        capture_button(const QString& text, QWidget* parent) : QPushButton(text, parent) {}
        boost::signals2::signal<void (HWND)> released_;

    private:
        virtual void mousePressEvent(QMouseEvent* event)
        {
            QPushButton::mousePressEvent(event);

            setCursor(Qt::CrossCursor);
        }

        virtual void mouseReleaseEvent(QMouseEvent* event)
        {
            QPushButton::mouseReleaseEvent(event);

            unsetCursor();
            QPoint point = QCursor::pos();
            POINT pt = { point.x(), point.y() };
            released_(WindowFromPoint(pt));
        }
    };

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
{
    auto widget = new QFrame(this);
    widget->setFrameStyle(QFrame::StyledPanel);

    setCentralWidget(widget);

    visualizer_ = new table_widget(this);

    capture_button* button = new capture_button("Capture", this);
    button->released_.connect([&](HWND hwnd) {
        site_.reset(new site_stars(hwnd));
        std::array<char, 256> arr;
        GetClassNameA(hwnd, &arr[0], int(arr.size()));
        QString class_name(&arr[0]);
        capture_label_->setText(QString("%1 (%2)").arg(class_name).arg(std::size_t(hwnd)));
    });

    decision_label_ = new QLabel("Decision: n/a", this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(visualizer_);
    layout->addWidget(decision_label_);
    layout->addWidget(button);
    widget->setLayout(layout);

    timer_ = new QTimer(this);
    connect(timer_, SIGNAL(timeout()), SLOT(timer_timeout()));
    timer_->start(100);

    create_menus();

    abstraction_label_ = new QLabel("No abstraction", this);
    statusBar()->addWidget(abstraction_label_, 1);
    strategy_label_ = new QLabel("No strategy", this);
    statusBar()->addWidget(strategy_label_, 1);
    capture_label_ = new QLabel("No window", this);
    statusBar()->addWidget(capture_label_, 1);
}

main_window::~main_window()
{
}

void main_window::timer_timeout()
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
    case site_stars::CALL:
        current_state = current_state->call();
        break;
    case site_stars::RAISE:
        current_state = current_state->raise(site_->get_raise_fraction());
        break;
    }

    if (!current_state)
        return;

    std::array<int, 2> pot = current_state->get_pot();
        
    if (site_->get_dealer() == 1)
        std::swap(pot[0], pot[1]);

    visualizer_->set_pot(current_state->get_round(), pot);

    if (!abstraction_)
        return;

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
        switch (site_->get_round())
        {
        case holdem_game::PREFLOP:
            bucket = abstraction_->get_bucket(c0, c1);
            break;
        case holdem_game::FLOP:
            bucket = abstraction_->get_bucket(c0, c1, b0, b1, b2);
            break;
        case holdem_game::TURN:
            bucket = abstraction_->get_bucket(c0, c1, b0, b1, b2, b3);
            break;
        case holdem_game::RIVER:
            bucket = abstraction_->get_bucket(c0, c1, b0, b1, b2, b3, b4);
            break;
        }
    }

    std::string s = "n/a";
    double probability = 0;
    auto& strategy = strategy_info.strategy_;

    if (current_state && bucket != -1)
    {
        const int action = strategy->get_action(current_state->get_id(), bucket);

        switch (action)
        {
        case nl_holdem_state::FOLD: s = "FOLD"; break;
        case nl_holdem_state::CALL: s = "CALL"; break;
        case nl_holdem_state::RAISE_HALFPOT: s = "RAISE_HALFPOT"; break;
        case nl_holdem_state::RAISE_75POT: s = "RAISE_75POT"; break;
        case nl_holdem_state::RAISE_POT: s = "RAISE_POT"; break;
        case nl_holdem_state::RAISE_MAX: s = "RAISE_MAX"; break;
        }

        probability = strategy->get(current_state->get_id(), action, bucket);
    }

    decision_label_->setText(QString("Decision: %1 (%2%)").arg(s.c_str()).arg(int(probability * 100)));
    strategy_label_->setText(QString("%1: %2").arg(stack_size).arg(QFileInfo(strategy->get_filename().c_str()).fileName()));
}

void main_window::create_menus()
{
    auto file_menu = menuBar()->addMenu("File");
    file_menu->addAction("Open abstraction...", this, SLOT(open_abstraction()));
    file_menu->addAction("Open strategy...", this, SLOT(open_strategy()));
}

void main_window::open_strategy()
{
    std::map<int, std::string> filenames;

    for (auto i = strategy_infos_.begin(); i != strategy_infos_.end(); ++i)
        filenames[i->first] = i->second->strategy_->get_filename();

    strategy_dialog d(filenames, this);

    if (d.exec() != QDialog::Accepted)
        return;

    strategy_infos_.clear();
    filenames = d.get_filenames();

    for (auto i = filenames.begin(); i != filenames.end(); ++i)
    {
        const auto stack_size = i->first;
        const auto filename = i->second;

        if (filename.empty())
            return;

        auto& si = strategy_infos_[stack_size];
        si.reset(new strategy_info);
        si->root_state_.reset(new nl_holdem_state(stack_size));
        si->current_state_ = si->root_state_.get();

        std::size_t states = 0;
        std::vector<const nl_holdem_state*> stack(1, si->root_state_.get());

        while (!stack.empty())
        {
            const nl_holdem_state* s = stack.back();
            stack.pop_back();

            if (!s->is_terminal())
                ++states;

            for (int i = nl_holdem_state::ACTIONS - 1; i >= 0; --i)
            {
                if (const nl_holdem_state* child = s->get_child(i))
                    stack.push_back(child);
            }
        }

        si->strategy_.reset(new strategy(filename, states, nl_holdem_state::ACTIONS));
    }
}

void main_window::open_abstraction()
{
    const auto filename = QFileDialog::getOpenFileName(this, "Open abstraction", QString(), "Abstraction files (*.abs)");

    if (filename.isEmpty())
        return;

    abstraction_.reset(new holdem_abstraction(std::string(filename.toUtf8().data())));
    abstraction_label_->setText(QFileInfo(filename).fileName());
}

main_window::strategy_info::strategy_info()
    : current_state_(nullptr)
{
}

main_window::strategy_info::~strategy_info()
{
}
