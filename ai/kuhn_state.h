#pragma once

class kuhn_state : private boost::noncopyable
{
public:
    enum kuhn_action
    {
        PASS = 0,
        BET = 1,
        ACTIONS
    };

    kuhn_state();
    kuhn_state(kuhn_state* parent, int action, int id);
    int get_id() const;
    int get_player() const;
    bool is_terminal() const;
    kuhn_state* act(int action, int id);
    kuhn_state* get_child(int action) const;
    int get_terminal_ev(int result) const;
    const kuhn_state* get_parent() const;
    int get_action() const;
    int get_num_actions() const;
    int get_round() const;

private:
    const int id_;
    kuhn_state* parent_;
    std::array<kuhn_state*, ACTIONS> children_;
    const int action_;
    const int player_;
    const int win_amount_;
    const bool terminal_;
    const int num_actions_;
};
