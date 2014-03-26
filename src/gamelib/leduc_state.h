#pragma once

#include <array>
#include <iostream>
#include <memory>
#include <vector>
#include "game_state_base.h"

class leduc_state : public game_state_base
{
public:
    enum game_round
    {
        PREFLOP,
        FLOP,
        ROUNDS,
    };

    enum leduc_action
    {
        FOLD,
        CALL,
        RAISE,
        ACTIONS
    };

    leduc_state();
    int get_action() const;
    game_round get_round() const;
    const leduc_state* get_parent() const;
    bool is_terminal() const;
    const leduc_state* get_child(int action) const;
    const leduc_state* get_action_child(int action) const;
    int get_id() const;
    int get_player() const;
    int get_terminal_ev(int result) const;
    int get_child_count() const;
    int get_action_count() const;

private:
    leduc_state(const leduc_state* parent, int action, int player, const std::array<int, 2>& pot, game_round round,
        int raises, int* id);
    void create_child(int action, int* id);

    const int id_;
    const leduc_state* parent_;
    std::vector<std::unique_ptr<leduc_state>> children_;
    const int action_;
    const int player_;
    const std::array<int, 2> pot_;
    const game_round round_;
    const int raises_;
};

std::ostream& operator<<(std::ostream& os, const leduc_state& state);
