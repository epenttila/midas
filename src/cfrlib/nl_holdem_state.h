#pragma once

#include <boost/noncopyable.hpp>
#include <array>

class nl_holdem_state : private boost::noncopyable
{
public:
    enum holdem_action
    {
        FOLD,
        CALL,
        RAISE_HALFPOT,
        RAISE_75POT,
        RAISE_POT,
        RAISE_MAX,
        ACTIONS
    };

    nl_holdem_state(int stack_size);
    int get_action() const;
    int get_round() const;
    const nl_holdem_state* get_parent() const;
    bool is_terminal() const;
    const nl_holdem_state* get_child(int action) const;
    int get_id() const;
    int get_player() const;
    int get_terminal_ev(int result) const;
    int get_child_count() const;

private:
    nl_holdem_state(const nl_holdem_state* parent, int action, int player, const std::array<int, 2>& pot, int round,
        int* id);
    void create_child(int action, int* id);

    const int id_;
    const nl_holdem_state* parent_;
    std::array<nl_holdem_state*, ACTIONS> children_;
    const int action_;
    const int player_;
    const std::array<int, 2> pot_;
    const int round_;
    int child_count_;
    const int stack_size_;
};

std::ostream& operator<<(std::ostream& os, const nl_holdem_state& state);
