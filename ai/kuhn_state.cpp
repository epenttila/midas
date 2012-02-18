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
}

int kuhn_state::get_actions() const
{
    return ACTIONS;
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

    children_.resize(ACTIONS);
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

double kuhn_state::get_terminal_ev(const std::array<int, 2>& cards) const
{
    if (!terminal_)
        throw std::runtime_error("not a terminal");

    if (parent_->action_ == action_) // PASS-PASS, BET-BET
        return cards[0] > cards[1] ? win_amount_ : -win_amount_;
    else if (parent_->action_ == BET && action_ == PASS)
        return (win_amount_ - 1) * (player_ == 0 ? 1 : -1);

    throw std::runtime_error("invalid terminal");
}

void kuhn_state::set_id(const int id)
{
    id_ = id;
}
