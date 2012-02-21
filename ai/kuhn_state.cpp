#include "common.h"
#include "kuhn_state.h"

kuhn_state::kuhn_state(kuhn_state* parent, const int action, const int win_amount)
    : id_(0)
    , parent_(parent)
    , children_()
    , action_(action)
    , player_(parent == nullptr ? 0 : 1 - parent->player_)
    , win_amount_(win_amount)
    , terminal_(parent != nullptr && (parent->action_ == action || parent->action_ == BET && action == PASS))
{
    children_.assign(nullptr);
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

kuhn_state* kuhn_state::act(const int action)
{
    if (terminal_)
        return nullptr;

    kuhn_state* next = children_[action];

    if (next == nullptr)
    {
        next = new kuhn_state(this, action, action_ == BET ? win_amount_ + 1 : win_amount_);
        children_[action] = next;
    }

    return next;
}

kuhn_state* kuhn_state::get_child(const int action) const
{
    return children_[action];
}

double kuhn_state::get_terminal_ev(const int result) const
{
    assert(terminal_);

    if (parent_->action_ == action_) // PASS-PASS, BET-BET
        return result * win_amount_;
    else if (parent_->action_ == BET && action_ == PASS)
        return player_ == 0 ? win_amount_ - 1 : 1 - win_amount_;

    assert(false);
    return 0;
}

void kuhn_state::set_id(const int id)
{
    id_ = id;
}

const kuhn_state* kuhn_state::get_parent() const
{
    return parent_;
}

int kuhn_state::get_action() const
{
    return action_;
}
