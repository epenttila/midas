#include "table_widget.h"

#pragma warning(push, 1)
#include <boost/lexical_cast.hpp>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <Windows.h>
#pragma warning(pop)

#include "util/card.h"

namespace
{
    static const int CARD_WIDTH = 50;
    static const int CARD_HEIGHT = 70;
    static const int CARD_MARGIN = 5;

    void paint_card(QPainter& painter, const QPoint& destination, const std::array<std::unique_ptr<QPixmap>, 52>& cards_images,
        const QPixmap& empty, int card)
    {
        if (card != -1)
            painter.drawPixmap(destination, *cards_images[card]);
        else
            painter.drawPixmap(destination, empty);
    }
}

table_widget::table_widget(QWidget* parent)
    : QFrame(parent)
    , dealer_image_(new QPixmap(":/images/dealer.png"))
    , dealer_(-1)
    , round_(-1)
    , empty_image_(new QPixmap(":/images/card_empty.png"))
{
    // TODO make editable and feed back to game state
    hole_[0].fill(-1);
    hole_[1].fill(-1);
    board_.fill(-1);
    bets_.fill(0);
    pot_.fill(0);

    for (int i = 0; i < 52; ++i)
        card_images_[i].reset(new QPixmap(QString(":/images/card_%1.png").arg(get_card_string(i).c_str())));

    setMinimumSize(13 * CARD_WIDTH + 15 * CARD_MARGIN, 3 * CARD_HEIGHT + 5 * CARD_MARGIN);
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
}

void table_widget::set_board_cards(const std::array<int, 5>& board)
{
    board_ = board;
    update();
}

void table_widget::set_hole_cards(int seat, const std::array<int, 2>& cards)
{
    hole_[seat] = cards;
    update();
}

void table_widget::set_dealer(int dealer)
{
    if (dealer == dealer_)
        return;

    dealer_ = dealer;
    bets_.fill(0);
    pot_.fill(0);
    round_ = 0;
    update();
}

void table_widget::set_pot(int round, const std::array<int, 2>& pot)
{
    if (round != 0 && round != round_)
    {
        pot_ = pot;
        bets_.fill(0);
        round_ = round;
    }
    else
    {
        bets_[0] = pot[0] - pot_[0];
        bets_[1] = pot[1] - pot_[1];
    }

    update();
}

void table_widget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    int y = CARD_MARGIN;

    paint_card(painter, QPoint(CARD_MARGIN, y), card_images_, *empty_image_, hole_[0][0]);
    paint_card(painter, QPoint(CARD_WIDTH + 2 * CARD_MARGIN, y), card_images_, *empty_image_, hole_[0][1]);
    paint_card(painter, QPoint(rect().right() - 2 * CARD_WIDTH - 2 * CARD_MARGIN, y), card_images_, *empty_image_, hole_[1][0]);
    paint_card(painter, QPoint(rect().right() - CARD_WIDTH - CARD_MARGIN, y), card_images_, *empty_image_, hole_[1][1]);

    for (int i = 0; i < board_.size(); ++i)
    {
        paint_card(painter, QPoint(rect().center().x() - (5 * CARD_WIDTH + 4 * CARD_MARGIN) / 2 + i
            * (CARD_WIDTH + CARD_MARGIN), y), card_images_, *empty_image_, board_[i]);
    }

    y += CARD_HEIGHT + CARD_MARGIN;

    painter.setFont(QFont("Helvetica", 16, QFont::Bold));

    if (bets_[0] > 0)
    {
        QRect rect(CARD_MARGIN, y, 2 * CARD_WIDTH + CARD_MARGIN, CARD_HEIGHT);
        painter.drawText(rect, Qt::AlignCenter, QString::number(bets_[0]));
    }

    if (bets_[1] > 0)
    {
        QRect rect(rect().right() - 2 * (CARD_WIDTH + CARD_MARGIN), y, 2 * CARD_WIDTH + CARD_MARGIN, CARD_HEIGHT);
        painter.drawText(rect, Qt::AlignCenter, QString::number(bets_[1]));
    }

    if (pot_[0] > 0)
    {
        QRect rect(rect().center().x() - 200, y, 400, CARD_HEIGHT);
        painter.drawText(rect, Qt::AlignCenter, QString::number(pot_[0] + pot_[1]));
    }

    y += CARD_HEIGHT + CARD_MARGIN;

    const int hole_midpoint = CARD_MARGIN + CARD_WIDTH + CARD_MARGIN / 2;
    const QPoint dealer_point((dealer_ == 0 ? hole_midpoint : rect().right() - hole_midpoint) - dealer_image_->width() / 2, y);

    painter.drawPixmap(dealer_point, *dealer_image_);

    QFrame::paintEvent(event);
}

void table_widget::get_board_cards(std::array<int, 5>& board) const
{
    board = board_;
}

void table_widget::get_hole_cards(std::array<int, 2>& hole) const
{
    hole = hole_[0];
}
