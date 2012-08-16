#pragma once

#pragma warning(push, 1)
#include <utility>
#include <array>
#include <QPixmap>
#pragma warning(pop)

class site_base
{
public:
    enum { FOLD, CALL, RAISE, ALLIN, SITOUT };
    enum { FOLD_BUTTON = 0x1, CALL_BUTTON = 0x2, RAISE_BUTTON = 0x4 };
    virtual ~site_base() {}
    virtual void update() = 0;
    virtual std::pair<int, int> get_hole_cards() const = 0;
    virtual void get_board_cards(std::array<int, 5>& board) const = 0;
    virtual int get_dealer() const = 0;
    virtual void fold() const = 0;
    virtual void call() const = 0;
    virtual void raise(double amount, double fraction) const = 0;
    virtual double get_stack(int player) const = 0;
    virtual double get_bet(int player) const = 0;
    virtual double get_big_blind() const = 0;
    virtual double get_total_pot() const = 0;
    virtual int get_player() const = 0;
    virtual bool is_opponent_allin() const = 0;
    virtual int get_buttons() const = 0;
    virtual bool is_opponent_sitout() const = 0;
    virtual bool is_sit_out() const = 0;
    virtual void sit_in() const = 0;
};
