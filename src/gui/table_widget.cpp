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
#include "util/game.h"

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
    , big_blind_(-1)
    , real_pot_(-1)
    , buttons_(-1)
{
    // TODO make editable and feed back to game state
    hole_[0].fill(-1);
    hole_[1].fill(-1);
    board_.fill(-1);
    bets_.fill(0);
    pot_.fill(0);
    real_bets_.fill(0);
    sit_out_.fill(false);
    stacks_.fill(0);

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

    const QRect left_pod(CARD_MARGIN, y, 2 * CARD_WIDTH + CARD_MARGIN, CARD_HEIGHT);
    const QRect right_pod(rect().right() - 2 * (CARD_WIDTH + CARD_MARGIN), y, 2 * CARD_WIDTH + CARD_MARGIN,
        CARD_HEIGHT);

    painter.drawText(left_pod, Qt::AlignCenter | Qt::AlignVCenter, QString("%1")
        .arg(sit_out_[0] ? "Sit out" : format_money(stacks_[0]).c_str()));
    painter.drawRect(left_pod);

    painter.drawText(right_pod, Qt::AlignCenter | Qt::AlignVCenter, QString("%1")
        .arg(sit_out_[1] ? "Sit out" : format_money(stacks_[1]).c_str()));
    painter.drawRect(right_pod);

    const QRect left_bet(left_pod.right() + CARD_MARGIN, y, left_pod.width(), left_pod.height());
    const QRect right_bet(right_pod.left() - CARD_MARGIN - right_pod.width(), y, right_pod.width(), right_pod.height());

    const QRect center_pot(rect().center().x() - 200, y, 400, CARD_HEIGHT);

    if (real_bets_[0] > 0)
    {
        painter.drawText(left_bet, Qt::AlignHCenter | Qt::AlignTop, QString("%1")
            .arg(format_money(real_bets_[0]).c_str()));
    }

    if (real_bets_[1] > 0)
    {
        painter.drawText(right_bet, Qt::AlignHCenter | Qt::AlignTop, QString("%1")
            .arg(format_money(real_bets_[1]).c_str()));
    }

    if (real_pot_ > 0)
    {
        painter.drawText(center_pot, Qt::AlignHCenter | Qt::AlignTop, QString("%1")
            .arg(format_money(real_pot_).c_str()));
    }

    if (bets_[0] > 0)
    {
        painter.drawText(left_bet, Qt::AlignHCenter | Qt::AlignBottom, QString("(%1)")
            .arg(format_money(bets_[0] * big_blind_ / 2.0).c_str()));
    }

    if (bets_[1] > 0)
    {
        painter.drawText(right_bet, Qt::AlignHCenter | Qt::AlignBottom, QString("(%1)")
            .arg(format_money(bets_[1] * big_blind_ / 2.0).c_str()));
    }

    if (pot_[0] > 0)
    {
        painter.drawText(center_pot, Qt::AlignHCenter | Qt::AlignBottom, QString("(%1)")
            .arg(format_money((pot_[0] + pot_[1]) * big_blind_ / 2.0).c_str()));
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

void table_widget::set_big_blind(double big_blind)
{
    big_blind_ = big_blind;
}

void table_widget::set_sit_out(bool sitout1, bool sitout2)
{
    sit_out_[0] = sitout1;
    sit_out_[1] = sitout2;
}

void table_widget::set_stacks(double stack1, double stack2)
{
    stacks_[0] = stack1;
    stacks_[1] = stack2;
}

void table_widget::set_buttons(int buttons)
{
    buttons_ = buttons;
}

void table_widget::set_real_bets(double bet1, double bet2)
{
    real_bets_[0] = bet1;
    real_bets_[1] = bet2;
}

void table_widget::set_real_pot(double pot)
{
    real_pot_ = pot;
}
