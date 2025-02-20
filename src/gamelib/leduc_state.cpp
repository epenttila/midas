#include "leduc_state.h"
#include <cassert>
#include <string>

namespace
{
    static const std::array<int, 2> INITIAL_POT = {{1, 1}};
}

leduc_state::leduc_state()
    : id_(0)
    , parent_(nullptr)
    , action_(-1)
    , player_(0)
    , pot_(INITIAL_POT)
    , round_(PREFLOP)
    , raises_(0) // blind counts as raise
{
    int id = id_ + 1;

    for (int i = 0; i < ACTIONS; ++i)
        create_child(i, &id);
}

leduc_state::leduc_state(const leduc_state* parent, const int action, const int player,
    const std::array<int, 2>& pot, const game_round round, const int raises, int* id)
    : id_(id ? (*id)++ : -1)
    , parent_(parent)
    , action_(action)
    , player_(player)
    , pot_(pot)
    , round_(round)
    , raises_(raises)
{
    for (int i = 0; i < ACTIONS; ++i)
        create_child(i, id);
}

void leduc_state::create_child(const int action, int* id)
{
    if (is_terminal())
        return;

    if (action == RAISE && raises_ == 2)
        return;

    if (action == FOLD && (action_ == CALL || parent_ == nullptr))
        return; // never fold if we can check

    assert(action_ != FOLD); // parent should be terminal if it was preceeded by folding

    bool new_terminal = false;

    if (action == FOLD)
        new_terminal = true; // folding always terminates
    else if (action == CALL && round_ == FLOP && parent_->round_ == FLOP)
        new_terminal = true; // river check or call is terminal

    game_round new_round = round_;

    if (action_ == RAISE && action == CALL)
        new_round = static_cast<game_round>(new_round + 1); // raise-call
    else if (action_ == CALL && action == CALL && parent_ && round_ == parent_->round_)
        new_round = static_cast<game_round>(new_round + 1); // check-check (or call-check preflop)

    std::array<int, 2> new_pot = {{pot_[0], pot_[1]}};

    if (action == CALL)
        new_pot[player_] = new_pot[1 - player_];
    else if (action == RAISE)
        new_pot[player_] = new_pot[1 - player_] + ((round_ == PREFLOP) ? 2 : 4);

    int new_player = 1 - player_;

    if (new_round != round_)
        new_player = 0; // sb begins new round

    int new_raises = raises_;

    if (round_ != new_round)
        new_raises = 0;
    else if (action == RAISE)
        ++new_raises;

    children_.push_back(std::unique_ptr<leduc_state>(new leduc_state(this, action, new_player, new_pot, new_round,
        new_raises, new_terminal ? nullptr : id)));
}

int leduc_state::get_terminal_ev(const int result) const
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

int leduc_state::get_action() const
{
    return action_;
}

leduc_state::game_round leduc_state::get_round() const
{
    return round_;
}

const leduc_state* leduc_state::get_parent() const
{
    return parent_;
}

bool leduc_state::is_terminal() const
{
    return id_ == -1;
}

const leduc_state* leduc_state::get_child(int action) const
{
    return children_[action].get();
}

int leduc_state::get_id() const
{
    return id_;
}

int leduc_state::get_player() const
{
    return player_;
}

int leduc_state::get_child_count() const
{
    return static_cast<int>(children_.size());
}

std::ostream& operator<<(std::ostream& os, const leduc_state& state)
{
    const char c[2][leduc_state::ACTIONS] = {{'f', 'c', 'r'}, {'F', 'C', 'R'}};
    std::string line;

    for (const leduc_state* s = &state; s->get_parent() != nullptr; s = s->get_parent())
        line = c[s->get_parent()->get_player()][s->get_action()] + line;

    return (os << state.get_id() << ":" << line);
}

int leduc_state::get_action_count() const
{
    return ACTIONS;
}

const leduc_state* leduc_state::get_action_child(int action) const
{
    return get_child(action);
}
