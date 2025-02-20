#pragma once

#include <array>
#include "game_state_base.h"

class kuhn_state : public game_state_base
{
public:
    enum game_round
    {
        FIRST = 0,
        ROUNDS,
    };

    enum kuhn_action
    {
        PASS = 0,
        BET = 1,
        ACTIONS
    };

    kuhn_state();
    int get_id() const;
    int get_player() const;
    bool is_terminal() const;
    kuhn_state* get_child(int action) const;
    const kuhn_state* get_action_child(int action) const;
    int get_terminal_ev(int result) const;
    const kuhn_state* get_parent() const;
    int get_action() const;
    int get_child_count() const;
    game_round get_round() const;
    int get_action_count() const;

private:
    typedef std::array<int, 2> pot_t;

    kuhn_state(kuhn_state* parent, int action, int player, const pot_t& pot, int* id);
    void create_child(int action, int* id);

    const int id_;
    kuhn_state* parent_;
    std::array<kuhn_state*, ACTIONS> children_;
    const int action_;
    const int player_;
    const pot_t pot_;
    int child_count_;
};

std::ostream& operator<<(std::ostream& os, const kuhn_state& state);
