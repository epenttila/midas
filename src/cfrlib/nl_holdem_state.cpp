#include "nl_holdem_state.h"
#include <cassert>
#include <string>
#include "holdem_game.h"

namespace
{
    static const std::array<int, 2> INITIAL_POT = {{1, 2}};

    int soft_translate(const double b1, const double b, const double b2)
    {
        const double s1 = (b1 / b - b1 / b2) / (1 - b1 / b2);
        const double s2 = (b / b2 - b1 / b2) / (1 - b1 / b2);
        return (rand() / RAND_MAX) < (s1 / (s1 + s2)) ? 0 : 1;
    }

    bool is_raise(int action)
    {
        return action > nl_holdem_state::CALL && action < nl_holdem_state::ACTIONS;
    }
}

nl_holdem_state::nl_holdem_state(const int stack_size)
    : id_(0)
    , parent_(nullptr)
    , action_(-1)
    , player_(0)
    , pot_(INITIAL_POT)
    , round_(holdem_game::PREFLOP)
    , child_count_(0) // sb has all actions
    , stack_size_(stack_size)
{
    children_.fill(nullptr);

    int id = id_ + 1;

    for (int i = 0; i < ACTIONS; ++i)
        create_child(i, &id);
}

nl_holdem_state::nl_holdem_state(const nl_holdem_state* parent, const int action, const int player,
    const std::array<int, 2>& pot, const int round, int* id)
    : id_(id ? (*id)++ : -1)
    , parent_(parent)
    , action_(action)
    , player_(player)
    , pot_(pot)
    , round_(round)
    , child_count_(0)
    , stack_size_(parent->stack_size_)
{
    children_.fill(nullptr);

    for (int i = 0; i < ACTIONS; ++i)
        create_child(i, id);
}

void nl_holdem_state::create_child(const int action, int* id)
{
    if (is_terminal())
        return;

    const bool opponent_allin = pot_[1 - player_] == stack_size_;

    if (is_raise(action) && opponent_allin)
        return;

    if (action == FOLD && action_ == CALL) // check -> fold
        return;

    assert(action_ != FOLD); // parent should be terminal if it was preceeded by folding

    bool new_terminal = false;

    if (action == FOLD)
        new_terminal = true; // folding always terminates
    else if (action == CALL && round_ == holdem_game::RIVER && parent_->round_ == holdem_game::RIVER)
        new_terminal = true; // river check or call is terminal
    else if (action == CALL && opponent_allin)
        new_terminal = true; // calling an all-in bet is terminal

    int new_round = round_;

    if (is_raise(action_) && action == CALL)
        ++new_round; // raise-call
    else if (action_ == CALL && action == CALL && parent_ && round_ == parent_->round_)
        ++new_round; // check-check (or call-check preflop)

    std::array<int, 2> new_pot = {{pot_[0], pot_[1]}};

    const int to_call = pot_[1 - player_] - pot_[player_];
    const int in_pot = pot_[1 - player_] + pot_[player_] - to_call;
    const int min_raise = (action_ == -1 ? 3 : (to_call == 0 ? 2 : 2 * to_call));

    assert(pot_[1 - player_] == stack_size_
        || action_ == -1
        || action_ == FOLD
        || action_ == CALL
        || (action_ == RAISE_HALFPOT && 2 * to_call == in_pot)
        || action_ == RAISE_75POT // don't check bet size due to rounding errors
        || (action_ == RAISE_POT && to_call == in_pot));

    switch (action)
    {
    case FOLD:
        break;
    case CALL:
        new_pot[player_] = new_pot[1 - player_];
        break;
    case RAISE_MAX:
        new_pot[player_] = stack_size_;
        break;
    default:
        {
            double factor;

            switch (action)
            {
            case RAISE_HALFPOT: factor = 0.5; break;
            case RAISE_75POT: factor = 0.75; break;
            case RAISE_POT: factor = 1.0; break;
            default: assert(false); factor = 1.0;
            }

            const int bet = int(to_call + (2 * to_call + in_pot) * factor);

            if (bet < min_raise)
                return; // combine all actions which are essentially CALL

            if (new_pot[player_] + bet >= stack_size_)
                return; // combine all actions which are essentially RAISE_MAX

            new_pot[player_] = new_pot[player_] + bet;
        }
        break;
    }

    int new_player;

    if (new_round != round_)
        new_player = 1; // bb begins new round
    else
        new_player = 1 - player_;

    assert(children_[action] == nullptr);

    children_[action] = new nl_holdem_state(this, action, new_player, new_pot, new_round, new_terminal ? nullptr : id);

    ++child_count_;
}

int nl_holdem_state::get_terminal_ev(const int result) const
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

int nl_holdem_state::get_action() const
{
    return action_;
}

int nl_holdem_state::get_round() const
{
    return round_;
}

const nl_holdem_state* nl_holdem_state::get_parent() const
{
    return parent_;
}

bool nl_holdem_state::is_terminal() const
{
    return id_ == -1;
}

const nl_holdem_state* nl_holdem_state::get_child(int action) const
{
    return children_[action];
}

int nl_holdem_state::get_id() const
{
    return id_;
}

int nl_holdem_state::get_player() const
{
    return player_;
}

int nl_holdem_state::get_child_count() const
{
    return child_count_;
}

std::ostream& operator<<(std::ostream& os, const nl_holdem_state& state)
{
    static const char actions[nl_holdem_state::ACTIONS] = {'f', 'c', 'h', 'q', 'p', 'a'};
    std::string line;

    for (const nl_holdem_state* s = &state; s->get_parent() != nullptr; s = s->get_parent())
    {
        const char c = actions[s->get_action()];
        line = char(s->get_parent()->get_player() == 0 ? c : toupper(c)) + line;
    }

    return (os << state.get_id() << ":" << line);
}

const nl_holdem_state* nl_holdem_state::call() const
{
    return children_[CALL];
}

const nl_holdem_state* nl_holdem_state::raise(const double fraction) const
{
    const double halfpot = 0.5 / 1.5;
    const double qpot = 0.75 / 1.75;
    const double pot = 1 / 2.0;

    int action;

    if (fraction <= halfpot)
        action = RAISE_HALFPOT;
    else if (fraction <= qpot)
        action = soft_translate(halfpot, fraction, qpot) == 0 ? RAISE_HALFPOT : RAISE_75POT;
    else if (fraction <= pot)
        action = soft_translate(qpot, fraction, pot) == 0 ? RAISE_75POT : RAISE_POT;
    else if (fraction <= 1)
        action = RAISE_POT;
    else
        action = RAISE_MAX;

    return children_[action];
}
