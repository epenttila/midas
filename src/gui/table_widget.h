#pragma once

#pragma warning(push, 3)
#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <QTableWidget>
#pragma warning(pop)

class QPixmap;

class table_widget : public QTableWidget
{
    Q_OBJECT

public:
    table_widget(QWidget* parent);
    void set_board_cards(WId window, const std::array<int, 5>& board);
    void set_hole_cards(WId window, const std::array<int, 2>& cards);
    void set_dealer(WId window, int dealer);
    void set_big_blind(WId window, double big_blind);
    void set_real_bets(WId window, double bet1, double bet2);
    void set_real_pot(WId window, double pot);
    void set_sit_out(WId window, bool sitout1, bool sitout2);
    void set_stacks(WId window, double stack1, double stack2);
    void set_buttons(WId window, int buttons);
    void set_tables(const std::unordered_set<WId>& tables);
    void set_active(WId window);
    void clear_row(WId window);
    void set_all_in(WId window, bool allin1, bool allin2);

private:
    int get_row(WId window) const;

    std::unique_ptr<QPixmap> dealer_image_;
    std::array<std::unique_ptr<QPixmap>, 52> card_images_;
    std::unique_ptr<QPixmap> empty_image_;
};
