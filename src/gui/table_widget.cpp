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

    void paint_card(QPainter& painter, const QPoint& destination, const QPixmap& cards_image, int card)
    {
        QRect rect;

        if (card != -1)
        {
            const int x = (12 - get_rank(card)) * CARD_WIDTH;
            int y;

            switch (get_suit(card))
            {
            case CLUB: y = 5 * CARD_HEIGHT; break;
            case DIAMOND: y = 4 * CARD_HEIGHT; break;
            case HEART: y = 70; break;
            case SPADE: y = 0; break;
            default: y = 0;
            }

            rect.setRect(x, y, CARD_WIDTH, CARD_HEIGHT);
        }
        else
        {
            rect.setRect(650, 140, CARD_WIDTH, CARD_HEIGHT);
        }

        painter.drawPixmap(destination, cards_image, rect);
    }
}

table_widget::table_widget(QWidget* parent)
    : QWidget(parent)
    , cards_image_(new QPixmap(":/images/cards.png"))
    , dealer_image_(new QPixmap(":/images/dealer.png"))
    , dealer_(-1)
    , round_(-1)
{
    hole_[0].fill(-1);
    hole_[1].fill(-1);
    board_.fill(-1);
    bets_.fill(0);
    pot_.fill(0);

    setFixedSize(800, 200);
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

void table_widget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    int y = 0;

    paint_card(painter, QPoint(0, y), *cards_image_, hole_[0][0]);
    paint_card(painter, QPoint(CARD_WIDTH + CARD_MARGIN, y), *cards_image_, hole_[0][1]);
    paint_card(painter, QPoint(rect().right() - 2 * CARD_WIDTH - CARD_MARGIN, y), *cards_image_, hole_[1][0]);
    paint_card(painter, QPoint(rect().right() - CARD_WIDTH, y), *cards_image_, hole_[1][1]);

    for (int i = 0; i < board_.size(); ++i)
    {
        paint_card(painter, QPoint(rect().center().x() - (5 * CARD_WIDTH + 4 * CARD_MARGIN) / 2 + i
            * (CARD_WIDTH + CARD_MARGIN), y), *cards_image_, board_[i]);
    }

    y += CARD_HEIGHT + CARD_MARGIN;

    painter.setFont(QFont("Helvetica", 16, QFont::Bold));

    if (bets_[0] > 0)
    {
        QRect rect(0, y, 2 * CARD_WIDTH + CARD_MARGIN, 4 * CARD_MARGIN);
        painter.drawText(rect, Qt::AlignCenter, QString::number(bets_[0]));
    }

    if (bets_[1] > 0)
    {
        QRect rect(rect().right() - (2 * CARD_WIDTH + CARD_MARGIN), y, 2 * CARD_WIDTH + CARD_MARGIN, 4 * CARD_MARGIN);
        painter.drawText(rect, Qt::AlignCenter, QString::number(bets_[1]));
    }

    if (pot_[0] > 0)
    {
        QRect rect(rect().center().x() - 200, y, 400, 4 * CARD_MARGIN);
        painter.drawText(rect, Qt::AlignCenter, QString::number(pot_[0] + pot_[1]));
    }

    y += CARD_MARGIN + 4 * CARD_MARGIN;

    const QPoint dealer_point((dealer_ == 0 ? CARD_WIDTH + CARD_MARGIN / 2 : rect().right() - (CARD_WIDTH + CARD_MARGIN
        / 2)) - dealer_image_->width() / 2, y);

    painter.drawPixmap(dealer_point, *dealer_image_);
}
