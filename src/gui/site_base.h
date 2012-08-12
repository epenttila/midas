#pragma once

#include <utility>
#include <array>

class site_base
{
public:
    enum { FOLD, CALL, RAISE, ALLIN };
    virtual ~site_base() {}
    virtual bool update() = 0;
    virtual int get_action() const = 0;
    virtual std::pair<int, int> get_hole_cards() const = 0;
    virtual void get_board_cards(std::array<int, 5>& board) const = 0;
    virtual bool is_new_hand() const = 0;
    virtual int get_round() const = 0;
    virtual double get_raise_fraction() const = 0;
    virtual int get_dealer() const = 0;
    virtual int get_stack_size() const = 0;
    virtual bool is_action_needed() const = 0;
    virtual void fold() const = 0;
    virtual void call() const = 0;
    virtual void raise(double fraction) const = 0;
};
