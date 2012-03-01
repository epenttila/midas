#include "common.h"
#include "holdem_state.h"

namespace
{
    static const std::array<int, 2> INITIAL_POT = {1, 2};
}

holdem_state::holdem_state()
    : id_(0)
    , parent_(nullptr)
    , action_(-1)
    , player_(0)
    , pot_(INITIAL_POT)
    , round_(PREFLOP)
    , raises_(1) // blind counts as raise
    , num_actions_(ACTIONS) // sb has all actions
{
    children_.fill(nullptr);
}

holdem_state::holdem_state(const holdem_state* parent, const int action, const int id, const int player,
    const std::array<int, 2>& pot, const int round, const int raises, const int num_actions)
    : id_(id)
    , parent_(parent)
    , action_(action)
    , player_(player)
    , pot_(pot)
    , round_(round)
    , raises_(raises)
    , num_actions_(num_actions)
{
    children_.fill(nullptr);
}

holdem_state* holdem_state::act(const int action, const int id)
{
    if (is_terminal())
        return nullptr;

    if (action == RAISE && raises_ == 4)
        return nullptr;

    assert(action_ != FOLD); // parent should be terminal if it was preceeded by folding

    bool new_terminal = false;

    if (action == FOLD)
        new_terminal = true; // folding always terminates
    else if (action == CALL && round_ == RIVER && parent_->round_ == RIVER)
        new_terminal = true; // river check or call is terminal

    int new_round = round_;

    if (action_ == RAISE && action == CALL)
        ++new_round; // raise-call
    else if (action_ == CALL && action == CALL && parent_ && round_ == parent_->round_)
        ++new_round; // check-check (or call-check preflop)

    std::array<int, 2> new_pot = {pot_[0], pot_[1]};

    if (action == CALL)
        new_pot[player_] = new_pot[1 - player_];
    else if (action == RAISE)
        new_pot[player_] = new_pot[1 - player_] + ((round_ <= FLOP) ? 2 : 4);

    int new_player = 1 - player_;

    if (new_round != round_)
        new_player = 1; // bb begins new round

    int new_raises = raises_;

    if (round_ != new_round)
        new_raises = 0;
    else if (action == RAISE)
        ++new_raises;

    int new_num_actions = 0;

    if (!new_terminal)
    {
        if (new_raises == 4)
            new_num_actions = ACTIONS - 1; // capped
        else
            new_num_actions = ACTIONS;
    }

    assert(children_[action] == nullptr);

    children_[action] = new holdem_state(this, action, new_terminal ? -1 : id, new_player, new_pot, new_round,
        new_raises, new_num_actions);

    return children_[action];
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

int holdem_state::get_num_actions() const
{
    return num_actions_;
}
