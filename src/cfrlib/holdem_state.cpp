#include "holdem_state.h"
#include <cassert>
#include <string>
#include "holdem_game.h"

namespace
{
    static const std::array<int, 2> INITIAL_POT = {{1, 2}};
}

holdem_state::holdem_state()
    : id_(0)
    , parent_(nullptr)
    , action_(-1)
    , player_(0)
    , pot_(INITIAL_POT)
    , round_(holdem_game::PREFLOP)
    , raises_(1) // blind counts as raise
    , child_count_(0) // sb has all actions
{
    children_.fill(nullptr);

    int id = id_ + 1;

    for (int i = 0; i < ACTIONS; ++i)
        create_child(i, &id);
}

holdem_state::holdem_state(const holdem_state* parent, const int action, const int player,
    const std::array<int, 2>& pot, const int round, const int raises, int* id)
    : id_(id ? (*id)++ : -1)
    , parent_(parent)
    , action_(action)
    , player_(player)
    , pot_(pot)
    , round_(round)
    , raises_(raises)
    , child_count_(0)
{
    children_.fill(nullptr);

    for (int i = 0; i < ACTIONS; ++i)
        create_child(i, id);
}

void holdem_state::create_child(const int action, int* id)
{
    if (is_terminal())
        return;

    if (action == RAISE && raises_ == 4)
        return;

    if (action == FOLD && action_ == CALL)
        return;

    assert(action_ != FOLD); // parent should be terminal if it was preceeded by folding

    bool new_terminal = false;

    if (action == FOLD)
        new_terminal = true; // folding always terminates
    else if (action == CALL && round_ == holdem_game::RIVER && parent_->round_ == holdem_game::RIVER)
        new_terminal = true; // river check or call is terminal

    int new_round = round_;

    if (action_ == RAISE && action == CALL)
        ++new_round; // raise-call
    else if (action_ == CALL && action == CALL && parent_ && round_ == parent_->round_)
        ++new_round; // check-check (or call-check preflop)

    std::array<int, 2> new_pot = {{pot_[0], pot_[1]}};

    if (action == CALL)
        new_pot[player_] = new_pot[1 - player_];
    else if (action == RAISE)
        new_pot[player_] = new_pot[1 - player_] + ((round_ <= holdem_game::FLOP) ? 2 : 4);

    int new_player = 1 - player_;

    if (new_round != round_)
        new_player = 1; // bb begins new round

    int new_raises = raises_;

    if (round_ != new_round)
        new_raises = 0;
    else if (action == RAISE)
        ++new_raises;

    assert(children_[action] == nullptr);

    children_[action] = new holdem_state(this, action, new_player, new_pot, new_round, new_raises,
        new_terminal ? nullptr : id);

    ++child_count_;
}

int holdem_state::get_terminal_ev(const int result) const
{
    assert(is_terminal());
    assert(action_ != CALL || pot_[0] == pot_[1]);

    if (action_ == FOLD)
        return parent_->player_ == 1 ? pot_[1] : -pot_[0];
    else if (action_ == CALL)
        return result * pot_[0];

    assert(false);
    return 0;
}

int holdem_state::get_action() const
{
    return action_;
}

int holdem_state::get_round() const
{
    return round_;
}

const holdem_state* holdem_state::get_parent() const
{
    return parent_;
}

bool holdem_state::is_terminal() const
{
    return id_ == -1;
}

const holdem_state* holdem_state::get_child(int action) const
{
    return children_[action];
}

int holdem_state::get_id() const
{
    return id_;
}

int holdem_state::get_player() const
{
    return player_;
}

int holdem_state::get_child_count() const
{
    return child_count_;
}

std::ostream& operator<<(std::ostream& os, const holdem_state& state)
{
    const char c[2][holdem_state::ACTIONS] = {{'f', 'c', 'r'}, {'F', 'C', 'R'}};
    std::string line;

    for (const holdem_state* s = &state; s->get_parent() != nullptr; s = s->get_parent())
        line = c[s->get_parent()->get_player()][s->get_action()] + line;

    return (os << state.get_id() << ":" << line);
}
