#pragma once

class kuhn_state : private boost::noncopyable
{
public:
    enum kuhn_action
    {
        PASS = 0,
        BET = 1,
        ACTIONS,
    };

    kuhn_state(kuhn_state* parent, const int action, const int win_amount);
    int get_id() const;
    int get_player() const;
    bool is_terminal() const;
    kuhn_state* act(const int action);
    kuhn_state* get_child(const int action) const;
    double get_terminal_ev(const int result) const;
    void set_id(const int id);
    const kuhn_state* get_parent() const;
    int get_action() const;

private:
    int id_;
    kuhn_state* parent_;
    std::array<kuhn_state*, ACTIONS> children_;
    const int action_;
    const int player_;
    const int win_amount_;
    const bool terminal_;
};
