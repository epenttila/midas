#include "holdem_strategy_widget.h"

#pragma warning(push, 1)
#include <string>
#include <cassert>
#include <QPainter>
#include <QApplication>
#pragma warning(pop)

#include "cfrlib/strategy.h"
#include "abslib/holdem_abstraction_base.h"
#include "util/game.h"

namespace
{
    static const int CELL_SIZE = 15;
    static const int CARDS = 52;
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
    static const std::array<std::string, 52> cards = {
        "2c", "2d", "2h", "2s",
        "3c", "3d", "3h", "3s",
        "4c", "4d", "4h", "4s",
        "5c", "5d", "5h", "5s",
        "6c", "6d", "6h", "6s",
        "7c", "7d", "7h", "7s",
        "8c", "8d", "8h", "8s",
        "9c", "9d", "9h", "9s",
        "Tc", "Td", "Th", "Ts",
        "Jc", "Jd", "Jh", "Js",
        "Qc", "Qd", "Qh", "Qs",
        "Kc", "Kd", "Kh", "Ks",
        "Ac", "Ad", "Ah", "As",
    };

    QPainter painter(this);
    painter.fillRect(rect(), QApplication::palette().color(QPalette::Window));
    painter.setPen(Qt::black);

    for (int i = 0; i < CARDS; ++i)
    {
        for (int j = 0; j < CARDS; ++j)
            painter.fillRect(QRect((i + 1) * CELL_SIZE, (j + 1) * CELL_SIZE, CELL_SIZE, CELL_SIZE), actions_[i][j]);
    }

    for (int i = 0; i < CARDS; ++i)
    {
        QRect r((i + 1) * CELL_SIZE, 0, CELL_SIZE, CELL_SIZE);
        painter.drawText(r, Qt::AlignCenter | Qt::AlignHCenter, QString(cards[i].c_str()));
        painter.drawLine(r.left(), rect().top(), r.left(), rect().bottom());
    }

    for (int i = 0; i < CARDS; ++i)
    {
        QRect r(0, (i + 1) * CELL_SIZE, CELL_SIZE, CELL_SIZE);
        painter.drawText(r, Qt::AlignCenter | Qt::AlignHCenter, QString(cards[i].c_str()));
        painter.drawLine(rect().left(), r.top(), rect().right(), r.top());
    }

    if (hole_[0] != -1 && hole_[1] != -1)
    {
        painter.setCompositionMode(QPainter::RasterOp_NotSourceXorDestination);
        painter.setPen(QPen(Qt::black, 3, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawRect((hole_[0] + 1) * CELL_SIZE, (hole_[1] + 1) * CELL_SIZE, CELL_SIZE, CELL_SIZE);
        painter.drawRect((hole_[1] + 1) * CELL_SIZE, (hole_[0] + 1) * CELL_SIZE, CELL_SIZE, CELL_SIZE);
    }
}

void holdem_strategy_widget::update(const holdem_abstraction_base& abs, const std::array<int, 2>& hole,
    const std::array<int, 5>& board, const strategy& strategy, std::size_t state_id, const int actions)
{
    hole_ = hole;

    for (int i = 0; i < actions_.size(); ++i)
        actions_[i].fill(Qt::transparent);

    for (int i = 0; i < CARDS; ++i)
    {
        if (i == board[0] || i == board[1] || i == board[2] || i == board[3] || i == board[4])
            continue;

        for (int j = 0; j < CARDS; ++j)
        {
            if (i == j || j == board[0] || j == board[1] || j == board[2] || j == board[3] || j == board[4])
                continue;

            int bucket = -1;

            holdem_abstraction_base::bucket_type buckets;
            abs.get_buckets(i, j, board[0], board[1], board[2], board[3], board[4], &buckets);

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

            for (int action = 0; action < actions; ++action)
            {
                const double p = strategy.get(state_id, action, bucket);

                if (action == 0)
                    fold_p = p;
                else if (action == 1)
                    call_p = p;
                else
                    raise_p += p;
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
