#pragma once

#pragma warning(push, 3)
#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <QTableWidget>
#pragma warning(pop)

#include "lobby_manager.h"

class QPixmap;

class table_widget : public QTableWidget
{
    Q_OBJECT

public:
    typedef lobby_manager::tid_t tid_t;

    table_widget(QWidget* parent);
    void set_board_cards(tid_t window, const std::array<int, 5>& board);
    void set_hole_cards(tid_t window, const std::array<int, 2>& cards);
    void set_dealer(tid_t window, int dealer);
    void set_big_blind(tid_t window, double big_blind);
    void set_real_bets(tid_t window, double bet1, double bet2);
    void set_real_pot(tid_t window, double pot);
    void set_sit_out(tid_t window, bool sitout1, bool sitout2);
    void set_stacks(tid_t window, double stack1, double stack2);
    void set_buttons(tid_t window, int buttons);
    void set_tables(const std::unordered_set<tid_t>& tables);
    void set_active(tid_t window);
    void clear_row(tid_t window);
    void set_all_in(tid_t window, bool allin1, bool allin2);

private:
    int get_row(tid_t window) const;

    std::unique_ptr<QPixmap> dealer_image_;
    std::array<std::unique_ptr<QPixmap>, 52> card_images_;
    std::unique_ptr<QPixmap> empty_image_;
};
