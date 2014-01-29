#pragma once

#include <array>

#include "nlhe_state_base.h"

class nlhe_state : public nlhe_state_base
{
public:
    static std::unique_ptr<nlhe_state> create(const std::string& config);

    nlhe_state(int stack_size, int enabled_actions, int limited_actions);
    holdem_action get_action() const;
    int get_round() const;
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
    std::string get_action_name(holdem_action action) const;
    int get_stack_size() const;

private:
    nlhe_state(const nlhe_state* parent, const holdem_action action_index, const int player,
        const std::array<int, 2>& pot, const int round, const std::array<int, 2> raise_masks, int enabled_actions,
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
    const int round_;
    const int stack_size_;
    const std::array<int, 2> raise_masks_;
    int limited_actions_;
    int enabled_actions_;
};
