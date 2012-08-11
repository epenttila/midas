#pragma once

#pragma warning(push, 3)
#include <array>
#include <regex>
#include <QWidget>
#pragma warning(pop)

#include "site_base.h"

class input_manager;

class site_888 : public site_base
{
public:
    site_888(WId window);
    bool update();
    int get_action() const;
    std::pair<int, int> get_hole_cards() const;
    void get_board_cards(std::array<int, 5>& board) const;
    bool is_new_hand() const;
    int get_round() const;
    double get_raise_fraction() const;
    int get_dealer() const;
    int get_stack_size() const;
    bool is_action_needed() const;
    void fold() const;
    void call() const;
    void raise(double amount) const;

private:
    int dealer_;
    int player_;
    int round_;
    double big_blind_;
    int action_;
    double stack_bb_;
    bool new_hand_;
    std::array<int, 2> hole_;
    std::array<int, 5> board_;
    double fraction_;
    WId window_;
    bool action_needed_;
    std::array<double, 2> bets_;
    double total_pot_;
    double to_call_;
    std::unique_ptr<input_manager> input_;
    bool can_raise_;
    std::regex big_blind_regex_;
};
