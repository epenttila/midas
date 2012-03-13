#pragma once

#include <boost/noncopyable.hpp>
#include <array>

class kuhn_state : private boost::noncopyable
{
public:
    enum kuhn_action
    {
        PASS = 0,
        BET = 1,
        ACTIONS
    };

    enum kuhn_round
    {
        FIRST,
        ROUNDS
    };

    kuhn_state();
    int get_id() const;
    int get_player() const;
    bool is_terminal() const;
    kuhn_state* get_child(int action) const;
    int get_terminal_ev(int result) const;
    const kuhn_state* get_parent() const;
    int get_action() const;
    int get_child_count() const;
    int get_round() const;

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
