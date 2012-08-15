#pragma once

#pragma warning(push, 3)
#include <array>
#include <QWidget>
#pragma warning(pop)

#include "site_base.h"

class site_stars : public site_base
{
public:
    site_stars(WId window);
    void update();
    std::pair<int, int> get_hole_cards() const;
    void get_board_cards(std::array<int, 5>& board) const;
    int get_dealer() const;
    void fold() const;
    void call() const;
    void raise(double amount, double fraction) const;
    double get_stack(int player) const;
    double get_bet(int player) const;
    double get_big_blind() const;
    double get_total_pot() const;
    int get_player() const;
    bool is_opponent_allin() const;
    int get_buttons() const;
    bool is_opponent_sitout() const;

private:
    int dealer_;
    int player_;
    int round_;
    double big_blind_;
    double stack_bb_;
    std::array<int, 2> hole_;
    std::array<int, 5> board_;
    double fraction_;
    WId window_;
};
