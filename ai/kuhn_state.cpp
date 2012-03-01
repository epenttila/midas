#include "common.h"
#include "kuhn_state.h"

kuhn_state::kuhn_state()
    : id_(0)
    , parent_(nullptr)
    , action_(-1)
    , player_(0)
    , win_amount_(1) // ante
    , terminal_(false)
    , num_actions_(ACTIONS)
{
    children_.fill(nullptr);
}

kuhn_state::kuhn_state(kuhn_state* parent, const int action, const int id)
    : id_(id)
    , parent_(parent)
    , children_()
    , action_(action)
    , player_(parent == nullptr ? 0 : 1 - parent->player_)
    , win_amount_(parent->get_action() == BET ? parent->win_amount_ + 1 : parent->win_amount_)
    , terminal_(parent != nullptr && (parent->action_ == action || (parent->action_ == BET && action == PASS)))
    , num_actions_(terminal_ ? 0 : ACTIONS)
{
    children_.fill(nullptr);
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
    return terminal_;
}

kuhn_state* kuhn_state::act(const int action, const int id)
{
    if (terminal_)
        return nullptr;

    assert(children_[action] == nullptr);
    children_[action] = new kuhn_state(this, action, id);
    return children_[action];
}

kuhn_state* kuhn_state::get_child(const int action) const
{
    return children_[action];
}

int kuhn_state::get_terminal_ev(const int result) const
{
    assert(terminal_);

    if (parent_->action_ == action_) // PASS-PASS, BET-BET
        return result * win_amount_;
    else if (parent_->action_ == BET && action_ == PASS)
        return player_ == 0 ? win_amount_ - 1 : 1 - win_amount_;

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

int kuhn_state::get_num_actions() const
{
    return num_actions_;
}

int kuhn_state::get_round() const
{
    return 0;
}
