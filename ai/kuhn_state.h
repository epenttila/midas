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
    int get_actions() const;
    int get_id() const;
    int get_player() const;
    bool is_terminal() const;
    kuhn_state* act(const int action);
    kuhn_state* get_child(const int action) const;
    double get_terminal_ev(const std::array<int, 2>& cards) const;
    void set_id(const int id);

private:
    int id_;
    kuhn_state* parent_;
    std::vector<kuhn_state*> children_;
    const int action_;
    const int player_;
    const int win_amount_;
    const bool terminal_;
};
