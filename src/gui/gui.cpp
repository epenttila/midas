#include "gui.h"

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
}

Gui::Gui()
    : root_state_(new nl_holdem_state(50))
    , current_state_(root_state_.get())
{
    auto widget = new QWidget(this);
    setCentralWidget(widget);

    text_ = new QTextEdit(this);
    text_->setReadOnly(true);

    capture_button* button = new capture_button("Capture", this);
    button->released_.connect([&](HWND hwnd) {
        site_.reset(new site_stars(hwnd));
        std::array<char, 256> arr;
        GetClassNameA(hwnd, &arr[0], int(arr.size()));
        QString class_name(&arr[0]);
        capture_label_->setText(QString("%1 (%2)").arg(class_name).arg(std::size_t(hwnd)));
    });

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(text_);
    layout->addWidget(button);
    widget->setLayout(layout);

    timer_ = new QTimer(this);
    connect(timer_, SIGNAL(timeout()), SLOT(timerTimeout()));
    timer_->start(100);

    create_menus();

    abstraction_label_ = new QLabel("No abstraction", this);
    statusBar()->addWidget(abstraction_label_, 1);
    strategy_label_ = new QLabel("No strategy", this);
    statusBar()->addWidget(strategy_label_, 1);
    capture_label_ = new QLabel("No window", this);
    statusBar()->addWidget(capture_label_, 1);
}

Gui::~Gui()
{
}

void Gui::timerTimeout()
{
    if (!abstraction_ || !strategy_ || !site_ || !site_->update())
        return;

    if (site_->is_new_hand())
        current_state_ = root_state_.get();

    if (current_state_)
    {
        switch (site_->get_action())
        {
        case site_stars::CALL:
            current_state_ = current_state_->call();
            break;
        case site_stars::RAISE:
            current_state_ = current_state_->raise(site_->get_raise_fraction());
            break;
        }
    }

    std::stringstream ss;

    if (current_state_)
        ss << *current_state_;

    auto hole = site_->get_hole_cards();
    const int c0 = hole.first;
    const int c1 = hole.second;

    std::array<int, 5> board;
    site_->get_board_cards(board);
    const int b0 = board[0];
    const int b1 = board[1];
    const int b2 = board[2];
    const int b3 = board[3];
    const int b4 = board[4];
    int bucket;

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
    default:
        bucket = -1;
    }

    std::string s;

    if (current_state_)
    {
        s += "FOLD: " + boost::lexical_cast<std::string>(strategy_->get(current_state_->get_id(), nl_holdem_state::FOLD, bucket)) + "\n";
        s += "CALL: " + boost::lexical_cast<std::string>(strategy_->get(current_state_->get_id(), nl_holdem_state::CALL, bucket)) + "\n";
        s += "RAISE_HALFPOT: " + boost::lexical_cast<std::string>(strategy_->get(current_state_->get_id(), nl_holdem_state::RAISE_HALFPOT, bucket)) + "\n";
        s += "RAISE_75POT: " + boost::lexical_cast<std::string>(strategy_->get(current_state_->get_id(), nl_holdem_state::RAISE_75POT, bucket)) + "\n";
        s += "RAISE_POT: " + boost::lexical_cast<std::string>(strategy_->get(current_state_->get_id(), nl_holdem_state::RAISE_POT, bucket)) + "\n";
        s += "RAISE_MAX: " + boost::lexical_cast<std::string>(strategy_->get(current_state_->get_id(), nl_holdem_state::RAISE_MAX, bucket)) + "\n";
        s += "\n";

        switch (strategy_->get_action(current_state_->get_id(), bucket))
        {
        case nl_holdem_state::FOLD: s += "FOLD\n"; break;
        case nl_holdem_state::CALL: s += "CALL\n"; break;
        case nl_holdem_state::RAISE_HALFPOT: s += "RAISE_HALFPOT\n"; break;
        case nl_holdem_state::RAISE_75POT: s += "RAISE_75POT\n"; break;
        case nl_holdem_state::RAISE_POT: s += "RAISE_POT\n"; break;
        case nl_holdem_state::RAISE_MAX: s += "RAISE_MAX\n"; break;
        }
    }

    text_->setText(QString("state: %1\nbucket: %2\n\n%3").arg(ss.str().c_str()).arg(bucket).arg(s.c_str()));
}

void Gui::create_menus()
{
    auto file_menu = menuBar()->addMenu("File");
    file_menu->addAction("Open abstraction...", this, SLOT(open_abstraction()));
    file_menu->addAction("Set strategies...", this, SLOT(open_strategy()));
}

void Gui::open_strategy()
{
    const auto filename = QFileDialog::getOpenFileName(this, "Open strategy", QString(), "Strategy files (*.str)");

    if (filename.isEmpty())
        return;

    std::size_t states = 0;
    std::vector<const nl_holdem_state*> stack(1, root_state_.get());

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

    strategy_.reset(new strategy(filename.toStdString(), states, nl_holdem_state::ACTIONS));
    strategy_label_->setText(QFileInfo(filename).fileName());
}

void Gui::open_abstraction()
{
    const auto filename = QFileDialog::getOpenFileName(this, "Open abstraction", QString(), "Abstraction files (*.abs)");

    if (filename.isEmpty())
        return;

    abstraction_.reset(new holdem_abstraction(std::ifstream(filename.toStdString(), std::ios::binary)));
    abstraction_label_->setText(QFileInfo(filename).fileName());
}
