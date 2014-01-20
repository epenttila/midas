#include "kuhn_state.h"
#include <cassert>
#include <string>
#include <iostream>

namespace
{
    static const std::array<int, 2> INITIAL_POT = {{1, 1}};
}

kuhn_state::kuhn_state()
    : id_(0)
    , parent_(nullptr)
    , action_(-1)
    , player_(0)
    , pot_(INITIAL_POT) // ante
    , child_count_(0)
{
    children_.fill(nullptr);

    int id = id_ + 1;

    for (int i = 0; i < ACTIONS; ++i)
        create_child(i, &id);
}

kuhn_state::kuhn_state(kuhn_state* parent, const int action, const int player, const pot_t& pot, int* id)
    : id_(id ? (*id)++ : -1)
    , parent_(parent)
    , children_()
    , action_(action)
    , player_(player)
    , pot_(pot)
    , child_count_(0)
{
    children_.fill(nullptr);

    for (int i = 0; i < ACTIONS; ++i)
        create_child(i, id);
}

int kuhn_state::get_id() const
{
    return id_;
}

int kuhn_state::get_player() const
{
    return player_;
}

bool kuhn_state::is_terminal() const
{
    return id_ == -1;
}

void kuhn_state::create_child(const int action, int* id)
{
    if (is_terminal())
        return;

    const int new_player = player_ ^ 1;
    const bool new_terminal = action_ == action || (action_ == BET && action == PASS);

    std::array<int, 2> new_pot = pot_;

    if (action == BET)
        ++new_pot[player_];

    assert(children_[action] == nullptr);

    children_[action] = new kuhn_state(this, action, new_player, new_pot, new_terminal ? nullptr : id);
    ++child_count_;
}

kuhn_state* kuhn_state::get_child(const int action) const
{
    return children_[action];
}

int kuhn_state::get_terminal_ev(const int result) const
{
    assert(is_terminal());

    if (parent_->action_ == action_) // PASS-PASS, BET-BET
        return result * pot_[0]; // showdown
    else if (parent_->action_ == BET && action_ == PASS)
        return parent_->player_ == 0 ? -pot_[0] : pot_[1]; // player folded

    assert(false);
    return 0;
}

const kuhn_state* kuhn_state::get_parent() const
{
    return parent_;
}

int kuhn_state::get_action() const
{
    return action_;
}

int kuhn_state::get_child_count() const
{
    return child_count_;
}

int kuhn_state::get_round() const
{
    return 0;
}

std::ostream& operator<<(std::ostream& os, const kuhn_state& state)
{
    const char c[2][kuhn_state::ACTIONS] = {{'p', 'b'}, {'p', 'B'}};
    std::string line;

    for (const kuhn_state* s = &state; s->get_parent() != nullptr; s = s->get_parent())
        line = c[s->get_parent()->get_player()][s->get_action()] + line;

    return (os << state.get_id() << ":" << line);
}
