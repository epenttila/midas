#pragma once

class holdem_state : private boost::noncopyable
{
public:
    enum holdem_action
    {
        FOLD,
        CALL,
        RAISE,
        ACTIONS
    };

    enum holdem_round
    {
        PREFLOP,
        FLOP,
        TURN,
        RIVER,
        ROUNDS
    };

    holdem_state();
    int get_action() const;
    int get_round() const;
    holdem_state* act(int action, int id);
    const holdem_state* get_parent() const;
    bool is_terminal() const;
    const holdem_state* get_child(int action) const;
    int get_id() const;
    int get_player() const;
    int get_terminal_ev(int result) const;
    int get_num_actions() const;

private:
    holdem_state(const holdem_state* parent, int action, int id, int player, const std::array<int, 2>& pot, int round,
        int raises, int num_actions);

    const int id_;
    const holdem_state* parent_;
    std::array<holdem_state*, ACTIONS> children_;
    const int action_;
    const int player_;
    const std::array<int, 2> pot_;
    const int round_;
    const int raises_;
    const int num_actions_;
};
