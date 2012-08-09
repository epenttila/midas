#pragma once

#include <boost/noncopyable.hpp>
#include <array>

class leduc_state : private boost::noncopyable
{
public:
    enum leduc_action
    {
        FOLD,
        CALL,
        RAISE,
        ACTIONS
    };

    leduc_state();
    int get_action() const;
    int get_round() const;
    const leduc_state* get_parent() const;
    bool is_terminal() const;
    const leduc_state* get_child(int action) const;
    int get_id() const;
    int get_player() const;
    int get_terminal_ev(int result) const;
    int get_child_count() const;

private:
    leduc_state(const leduc_state* parent, int action, int player, const std::array<int, 2>& pot, int round,
        int raises, int* id);
    void create_child(int action, int* id);

    const int id_;
    const leduc_state* parent_;
    std::array<leduc_state*, ACTIONS> children_;
    const int action_;
    const int player_;
    const std::array<int, 2> pot_;
    const int round_;
    const int raises_;
    int child_count_;
};

std::ostream& operator<<(std::ostream& os, const leduc_state& state);
