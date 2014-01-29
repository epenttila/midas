#pragma once

#include <array>
#include <memory>
#include <iostream>

#include "holdem_state.h"

class nlhe_state : public holdem_state
{
public:
    enum
    {
        F_MASK = 0x001,
        C_MASK = 0x002,
        O_MASK = 0x004,
        H_MASK = 0x008,
        Q_MASK = 0x010,
        P_MASK = 0x020,
        W_MASK = 0x040,
        D_MASK = 0x080,
        V_MASK = 0x100,
        T_MASK = 0x200,
        A_MASK = 0x400,
    };

    enum holdem_action
    {
        INVALID_ACTION = -1,
        FOLD = 0,
        CALL = 1,
        RAISE_O = 2,
        RAISE_H = 3,
        RAISE_Q = 4,
        RAISE_P = 5,
        RAISE_W = 6,
        RAISE_D = 7,
        RAISE_V = 8,
        RAISE_T = 9,
        RAISE_A = 10,
        ACTIONS,
    };

    static double get_raise_factor(const holdem_action action);
    static std::unique_ptr<nlhe_state> create(const std::string& config);
    static std::string get_action_name(holdem_action action);

    nlhe_state(int stack_size, int enabled_actions, int limited_actions);
    holdem_action get_action() const;
    game_round get_round() const;
    const nlhe_state* get_parent() const;
    bool is_terminal() const;
    const nlhe_state* get_child(int index) const;
    const nlhe_state* get_action_child(int action) const;
    int get_id() const;
    int get_player() const;
    int get_terminal_ev(int result) const;
    int get_child_count() const;
    const nlhe_state* call() const;
    const nlhe_state* raise(double fraction) const;
    std::array<int, 2> get_pot() const;
    int get_stack_size() const;

private:
    nlhe_state(const nlhe_state* parent, const holdem_action action_index, const int player,
        const std::array<int, 2>& pot, const game_round round, const std::array<int, 2> raise_masks, int enabled_actions,
        int limited_actions, int* id);
    void create_child(holdem_action action, int* id);
    bool is_raise(holdem_action action) const;
    bool is_action_enabled(holdem_action action) const;
    int get_new_player_pot(int player_pot, int to_call, int in_pot, holdem_action action) const;

    const int id_;
    const nlhe_state* parent_;
    std::vector<std::unique_ptr<nlhe_state>> children_;
    const holdem_action action_;
    const int player_;
    const std::array<int, 2> pot_;
    const game_round round_;
    const int stack_size_;
    const std::array<int, 2> raise_masks_;
    int limited_actions_;
    int enabled_actions_;
};

std::ostream& operator<<(std::ostream& os, const nlhe_state& state);
