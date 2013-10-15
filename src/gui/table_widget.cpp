#include "table_widget.h"

#pragma warning(push, 1)
#include <boost/lexical_cast.hpp>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <Windows.h>
#pragma warning(pop)

#include "util/card.h"
#include "util/game.h"

namespace
{
    enum
    {
        WINDOW_COLUMN,
        BIGBLIND_COLUMN,
        PSTACK_COLUMN,
        OSTACK_COLUMN,
        PSITOUT_COLUMN,
        OSITOUT_COLUMN,
        HOLE_COLUMN,
        DEALER_COLUMN,
        BOARD_COLUMN,
        PBET_COLUMN,
        OBET_COLUMN,
        POT_COLUMN,
        BUTTONS_COLUMN,
        MAX_COLUMNS,
    };

    static const int CARD_WIDTH = 50;
    static const int CARD_HEIGHT = 70;
    static const int CARD_MARGIN = 5;

    class card_delegate : public QAbstractItemDelegate
    {
    public:
        card_delegate(const std::array<std::unique_ptr<QPixmap>, 52>& images)
            : images_(images)
        {
        }

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            const auto list = index.data(Qt::DisplayRole).toList();

            const int y = option.rect.top() + (option.rect.height() - images_[0]->height()) / 2;
            int x = option.rect.left();

            painter->setClipRect(option.rect);

            for (auto i : list)
            {
                if (i.toInt() != -1)
                    painter->drawPixmap(x, y, *images_[i.toInt()]);

                x += images_[0]->width();
            }
        }

        QSize sizeHint(const QStyleOptionViewItem&,const QModelIndex& index) const
        {
            const auto list = index.data(Qt::DisplayRole).toList();

            return QSize(images_[0]->width() * list.size(), images_[0]->height());
        }

    private:
        const std::array<std::unique_ptr<QPixmap>, 52>& images_;
    };
}

table_widget::table_widget(QWidget* parent)
    : QTableWidget(0, MAX_COLUMNS, parent)
    , dealer_image_(new QPixmap(":/images/dealer.png"))
    , empty_image_(new QPixmap(":/images/card_empty.png"))
{
    for (int i = 0; i < 52; ++i)
        card_images_[i].reset(new QPixmap(QString(":/images/mini_%1.png").arg(get_card_string(i).c_str())));

    setMinimumSize(13 * CARD_WIDTH + 15 * CARD_MARGIN, 3 * CARD_HEIGHT + 5 * CARD_MARGIN);

    setHorizontalHeaderLabels(QStringList() << "Window" << "BB" << "PStack" << "OStack" << "PSitout" << "OSitout"
        << "Hole" << "Dealer" << "Board" << "PBet" << "OBet" << "Pot" << "Buttons");

    setItemDelegateForColumn(HOLE_COLUMN, new card_delegate(card_images_));
    setItemDelegateForColumn(BOARD_COLUMN, new card_delegate(card_images_));

    resizeColumnsToContents();
}

void table_widget::set_board_cards(WId window, const std::array<int, 5>& board)
{
    QList<QVariant> list;

    for (auto i : board)
        list.append(i);

    item(get_row(window), BOARD_COLUMN)->setData(Qt::DisplayRole, list);

    resizeColumnToContents(BOARD_COLUMN);
}

void table_widget::set_hole_cards(WId window, const std::array<int, 2>& cards)
{
    QList<QVariant> list;

    for (auto i : cards)
        list.append(i);

    item(get_row(window), HOLE_COLUMN)->setData(Qt::DisplayRole, list);

    resizeColumnToContents(HOLE_COLUMN);
}

void table_widget::set_dealer(WId window, int dealer)
{
    item(get_row(window), DEALER_COLUMN)->setData(Qt::DisplayRole, dealer);
}

void table_widget::set_pot(WId, int, const std::array<int, 2>&)
{
    //item(get_row(window), DEALER_COLUMN)->setData(Qt::DisplayRole, dealer);
}

void table_widget::set_big_blind(WId window, double big_blind)
{
    item(get_row(window), BIGBLIND_COLUMN)->setData(Qt::DisplayRole, big_blind);
}

void table_widget::set_sit_out(WId window, bool sitout1, bool sitout2)
{
    item(get_row(window), PSITOUT_COLUMN)->setData(Qt::DisplayRole, sitout1);
    item(get_row(window), OSITOUT_COLUMN)->setData(Qt::DisplayRole, sitout2);
}

void table_widget::set_stacks(WId window, double stack1, double stack2)
{
    item(get_row(window), PSTACK_COLUMN)->setData(Qt::DisplayRole, stack1);
    item(get_row(window), OSTACK_COLUMN)->setData(Qt::DisplayRole, stack2);
}

void table_widget::set_buttons(WId window, int buttons)
{
    item(get_row(window), BUTTONS_COLUMN)->setData(Qt::DisplayRole, buttons);
}

void table_widget::set_real_bets(WId window, double bet1, double bet2)
{
    item(get_row(window), PBET_COLUMN)->setData(Qt::DisplayRole, bet1);
    item(get_row(window), OBET_COLUMN)->setData(Qt::DisplayRole, bet2);
}

void table_widget::set_real_pot(WId window, double pot)
{
    item(get_row(window), POT_COLUMN)->setData(Qt::DisplayRole, pot);
}

void table_widget::set_tables(const std::unordered_set<WId>& tables)
{
    std::unordered_set<WId> existing;

    for (int row = 0; row < rowCount(); ++row)
    {
        const auto window = item(row, WINDOW_COLUMN)->data(Qt::DisplayRole).toULongLong();

        if (tables.find(window) != tables.end())
        {
            existing.insert(window);
            continue;
        }

        removeRow(row--);
    }

    for (auto window : tables)
    {
        if (existing.find(window) != existing.end())
            continue;

        auto row = rowCount();
        insertRow(row);

        for (int col = 0; col < MAX_COLUMNS; ++col)
            setItem(row, col, new QTableWidgetItem);

        item(row, WINDOW_COLUMN)->setData(Qt::DisplayRole, window);
    }
}

int table_widget::get_row(WId window) const
{
    for (int row = 0; row < rowCount(); ++row)
    {
        if (window == item(row, WINDOW_COLUMN)->data(Qt::DisplayRole).toULongLong())
            return row;
    }

    return -1;
}

void table_widget::set_active(WId window)
{
    const auto row = get_row(window);

    if (row == -1)
        return;

    selectRow(row);
}
