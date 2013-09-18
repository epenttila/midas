#include "holdem_strategy_widget.h"

#pragma warning(push, 1)
#include <string>
#include <cassert>
#include <QPainter>
#include <QApplication>
#include <QHelpEvent>
#include <QToolTip>
#pragma warning(pop)

#include "cfrlib/strategy.h"
#include "abslib/holdem_abstraction_base.h"
#include "util/game.h"
#include "util/card.h"
#include "cfrlib/nlhe_strategy.h"
#include "cfrlib/nlhe_state.h"

namespace
{
    static const int CELL_SIZE = 15;

    int get_horz_card(int card)
    {
        return CELL_SIZE + card * CELL_SIZE;
    }

    int get_vert_card(int card)
    {
        return card * CELL_SIZE;
    }
}

holdem_strategy_widget::holdem_strategy_widget(QWidget* parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
{
    for (int i = 0; i < actions_.size(); ++i)
        actions_[i].fill(Qt::transparent);

    setFixedSize(CELL_SIZE * (1 + CARDS), CELL_SIZE * (1 + CARDS));
}

void holdem_strategy_widget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), QApplication::palette().color(QPalette::Window));
    painter.setPen(Qt::black);

    for (int i = 0; i < CARDS; ++i)
    {
        for (int j = i + 1; j < CARDS; ++j)
            painter.fillRect(QRect(get_horz_card(i), get_vert_card(j), CELL_SIZE, CELL_SIZE), actions_[i][j]);
    }

    for (int i = 0; i < CARDS; ++i)
    {
        QRect r(get_horz_card(i), rect().bottom() - CELL_SIZE, CELL_SIZE, CELL_SIZE);
        painter.drawText(r, Qt::AlignCenter | Qt::AlignHCenter, QString(get_card_string(i).c_str()));
    }

    for (int i = 0; i < CARDS; ++i)
    {
        QRect r(0, get_vert_card(i), CELL_SIZE, CELL_SIZE);
        painter.drawText(r, Qt::AlignCenter | Qt::AlignHCenter, QString(get_card_string(i).c_str()));
    }

    if (hole_[0] != -1 && hole_[1] != -1)
    {
        painter.setCompositionMode(QPainter::RasterOp_NotSourceXorDestination);
        painter.setPen(QPen(Qt::black, 3, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawRect(get_horz_card(hole_[0]), get_vert_card(hole_[1]), CELL_SIZE, CELL_SIZE);
    }
}

void holdem_strategy_widget::update(const nlhe_strategy& strategy, const std::array<int, 2>& hole,
    const std::array<int, 5>& board, std::size_t state_id)
{
    hole_ = hole;

    if (hole_[0] > hole_[1])
        std::swap(hole_[0], hole_[1]);

    for (int i = 0; i < actions_.size(); ++i)
        actions_[i].fill(Qt::transparent);

    for (int i = 0; i < CARDS; ++i)
    {
        if (i == board[0] || i == board[1] || i == board[2] || i == board[3] || i == board[4])
            continue;

        for (int j = i + 1; j < CARDS; ++j)
        {
            if (j == board[0] || j == board[1] || j == board[2] || j == board[3] || j == board[4])
                continue;

            int bucket = -1;

            holdem_abstraction_base::bucket_type buckets;
            strategy.get_abstraction().get_buckets(i, j, board[0], board[1], board[2], board[3], board[4], &buckets);

            if (board[4] != -1)
                bucket = buckets[RIVER];
            else if (board[3] != -1)
                bucket = buckets[TURN];
            else if (board[2] != -1)
                bucket = buckets[FLOP];
            else
                bucket = buckets[PREFLOP];

            double fold_p = 0;
            double call_p = 0;
            double raise_p = 0;

            action_names_[i][j].clear();

            for (int index = 0; index < strategy.get_root_state().get_action_count(); ++index)
            {
                const double p = strategy.get_strategy().get(state_id, index, bucket);
                const auto action = strategy.get_root_state().get_action(index);

                if (action == 0)
                    fold_p = p;
                else if (action == 1)
                    call_p = p;
                else
                    raise_p += p;

                if (action > 0)
                    action_names_[i][j] += "\n";

                action_names_[i][j] += QString("%1 (%2)")
                    .arg(strategy.get_root_state().get_action_name(action).c_str()).arg(p);
            }

            fold_p = std::min(fold_p, 1.0);
            call_p = std::min(call_p, 1.0);
            raise_p = std::min(raise_p, 1.0);

            assert(std::abs(fold_p + call_p + raise_p - 1.0) < 0.0001);
            actions_[i][j] = QColor::fromRgbF(raise_p, call_p, fold_p);
        }
    }

    QWidget::update();
}

bool holdem_strategy_widget::event(QEvent* event)
{
    if (event->type() == QEvent::ToolTip)
    {
        QHelpEvent* helpEvent = dynamic_cast<QHelpEvent*>(event);

        const int i = helpEvent->pos().x() / CELL_SIZE - 1;
        const int j = helpEvent->pos().y() / CELL_SIZE;

        if (i >= 0 && i < CARDS && j >= 0 && j < CARDS && i < j)
        {
            auto str = QString("%1 %2").arg(get_card_string(i).c_str()).arg(get_card_string(j).c_str());

            if (!action_names_[i][j].isEmpty())
                str += "\n\n" + action_names_[i][j];

            QToolTip::showText(helpEvent->globalPos(), str);
        }
        else
        {
            QToolTip::hideText();
            event->ignore();
        }

        return true;
    }

    return QWidget::event(event);
}
