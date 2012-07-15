#pragma warning(push, 3)
#include <QtGui>
#pragma warning(pop)

#include "gui.h"
#include <array>
#include <fstream>
#include <sstream>
#include "abslib/holdem_abstraction.h"
#include "site_stars.h"
#include "util/card.h"
#include "cfrlib/holdem_game.h"
#include "cfrlib/nl_holdem_state.h"

Gui::Gui()
    : root_state_(new nl_holdem_state(50))
    , current_state_(root_state_.get())
    , site_(new site_stars)
{
    text_ = new QTextEdit(this);
    text_->setReadOnly(true);

    QPushButton* button = new QPushButton("Start", this);
    button->setCheckable(true);

    connect(button, SIGNAL(clicked()), SLOT(buttonClicked()));

    timer_ = new QTimer(this);
    connect(timer_, SIGNAL(timeout()), SLOT(timerTimeout()));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(text_);
    layout->addWidget(button);

    setLayout(layout);

    std::string abstraction_filename = QFileDialog::getOpenFileName(this).toStdString();
    abstraction_.reset(new holdem_abstraction(std::ifstream(abstraction_filename, std::ios::binary)));
}

Gui::~Gui()
{
}

void Gui::buttonClicked()
{
    if (!timer_->isActive())
        timer_->start(100);
    else
        timer_->stop();
}

void Gui::timerTimeout()
{
    if (!site_->update())
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

    text_->setText(QString("state: %1\nbucket: %2\n").arg(ss.str().c_str()).arg(bucket));
}
