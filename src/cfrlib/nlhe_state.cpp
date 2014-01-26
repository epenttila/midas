#include "nlhe_state.h"
#include <boost/integer/static_log2.hpp>
#include <random>
#include <boost/regex.hpp>
#include "util/game.h"
#include "holdem_game.h"

namespace detail
{
    static const std::array<int, 2> INITIAL_RAISE_MASKS = {{0, 0}};
}

nlhe_state::nlhe_state(int stack_size, int enabled_actions, int limited_actions)
    : id_(0)
    , parent_(nullptr)
    , action_(INVALID_ACTION)
    , player_(0)
    , pot_(detail::INITIAL_POT)
    , round_(PREFLOP)
    , stack_size_(stack_size)
    , raise_masks_(detail::INITIAL_RAISE_MASKS)
    , limited_actions_(limited_actions)
    , enabled_actions_(enabled_actions)
{
    int id = id_ + 1;

    for (int i = 0; i < ACTIONS; ++i)
        create_child(static_cast<holdem_action>(i), &id);
}

nlhe_state::nlhe_state(const nlhe_state* parent, const holdem_action action_index, const int player,
    const std::array<int, 2>& pot, const int round, const std::array<int, 2> raise_masks, int enabled_actions,
    int limited_actions, int* id)
    : id_(id ? (*id)++ : -1)
    , parent_(parent)
    , action_(action_index)
    , player_(player)
    , pot_(pot)
    , round_(round)
    , stack_size_(parent->stack_size_)
    , raise_masks_(raise_masks)
    , limited_actions_(limited_actions)
    , enabled_actions_(enabled_actions)
{
    for (int i = 0; i < ACTIONS; ++i)
        create_child(static_cast<holdem_action>(i), id);
}

void nlhe_state::create_child(const holdem_action action_index, int* id)
{
    if (!is_action_enabled(action_index))
        return;

    if (is_terminal())
        return;

    const bool opponent_allin = pot_[1 - player_] == stack_size_;
    const holdem_action next_action = action_index;
    const holdem_action prev_action = action_;

    if (is_raise(next_action) && opponent_allin)
        return;

    if (next_action == FOLD && prev_action == CALL) // prevent check -> fold
        return;

    assert(prev_action != FOLD); // parent should be terminal if it was preceeded by folding

    bool new_terminal = false;

    if (next_action == FOLD)
        new_terminal = true; // folding always terminates
    else if (next_action == CALL && round_ == RIVER && parent_->round_ == RIVER)
        new_terminal = true; // river check or call is terminal
    else if (next_action == CALL && opponent_allin)
        new_terminal = true; // calling an all-in bet is terminal

    int new_round = round_;

    if (is_raise(prev_action) && next_action == CALL)
        ++new_round; // raise-call
    else if (prev_action == CALL && next_action == CALL && parent_ && round_ == parent_->round_)
        ++new_round; // check-check (or call-check preflop)

    // restrict small raises (< RAISE_P) to 1 per player per round
    std::array<int, 2> new_raise_masks;

    if (new_round == round_)
        new_raise_masks = raise_masks_;
    else
        new_raise_masks.fill(0);

    if (next_action > CALL && next_action < RAISE_P)
    {
        const auto mask = get_action_mask(next_action);

        if (limited_actions_ & mask)
        {
            if ((new_raise_masks[player_] & mask) == mask)
                return;
            else
                new_raise_masks[player_] |= mask;
        }
    }

    std::array<int, 2> new_pot = {{pot_[0], pot_[1]}};

    const int to_call = pot_[1 - player_] - pot_[player_];
    const int in_pot = pot_[1 - player_] + pot_[player_] - to_call;

    assert(pot_[1 - player_] == stack_size_
        || prev_action == -1
        || prev_action == FOLD
        || prev_action == CALL
        || (prev_action == RAISE_O && to_call == std::max(to_call == 0 ? 2 : to_call, static_cast<int>(std::ceil(0.25 * in_pot))))
        || (prev_action == RAISE_H && to_call == std::ceil(0.5 * in_pot))
        || (prev_action == RAISE_Q && to_call == std::ceil(0.75 * in_pot))
        || (prev_action == RAISE_P && to_call == in_pot)
        || (prev_action == RAISE_W && to_call == std::ceil(1.5 * in_pot))
        || (prev_action == RAISE_D && to_call == 2 * in_pot)
        || (prev_action == RAISE_V && to_call == 5 * in_pot)
        || (prev_action == RAISE_T && to_call == 10 * in_pot));

    switch (next_action)
    {
    case FOLD:
        break;
    case CALL:
        new_pot[player_] = pot_[1 - player_];
        break;
    default:
        {
            int new_player_pot = get_new_player_pot(pot_[player_], to_call, in_pot, next_action);

            // combine similar actions upwards
            int next_size_action = next_action;
            
            if (next_action != RAISE_A)
            {
                do
                {
                    ++next_size_action;
                }
                while (!is_action_enabled(static_cast<holdem_action>(next_size_action)));
            }

            const int max_player_pot = get_new_player_pot(pot_[player_], to_call, in_pot, static_cast<holdem_action>(next_size_action));

            if (new_player_pot < max_player_pot)
            {
                if (prev_action == INVALID_ACTION)
                    new_player_pot = std::max(pot_[player_] + 3, new_player_pot);
                else
                    new_player_pot = std::max(pot_[player_] + (to_call == 0 ? 2 : 2 * to_call), new_player_pot);
            }

            if (new_player_pot >= max_player_pot && next_action != next_size_action)
                return; // combine all actions which are essentially RAISE_MAX

            // support combining minraises with CALL if this fails
            assert(!(new_player_pot < max_player_pot && new_player_pot - pot_[player_]
                < (prev_action == -1 ? 3 : (to_call == 0 ? 2 : 2 * to_call))));

            new_pot[player_] = new_player_pot;
        }
        break;
    }

    int new_player;

    if (new_round != round_)
        new_player = 1; // bb begins new round
    else
        new_player = 1 - player_;

    children_.push_back(std::unique_ptr<nlhe_state>(new nlhe_state(this, action_index, new_player, new_pot, new_round,
        new_raise_masks, enabled_actions_, limited_actions_, new_terminal ? nullptr : id)));
}

int nlhe_state::get_terminal_ev(const int result) const
{
    assert(is_terminal());
    assert(action_ != CALL || pot_[0] == pot_[1]);

    if (action_ == FOLD)
    {
        return parent_->player_ == 1 ? pot_[1] : -pot_[0];
    }
    else if (action_ == CALL)
    {
        assert(pot_[0] == pot_[1]);
        return result * pot_[0];
    }

    assert(false);
    return 0;
}

nlhe_state::holdem_action nlhe_state::get_action() const
{
    return action_;
}

int nlhe_state::get_round() const
{
    return round_;
}

const nlhe_state* nlhe_state::get_parent() const
{
    return parent_;
}

bool nlhe_state::is_terminal() const
{
    return id_ == -1;
}

const nlhe_state* nlhe_state::get_child(int index) const
{
    if (index == -1)
        return nullptr;

    return children_[index].get();
}

int nlhe_state::get_id() const
{
    return id_;
}

int nlhe_state::get_player() const
{
    return player_;
}

int nlhe_state::get_child_count() const
{
    return static_cast<int>(children_.size());
}

const nlhe_state* nlhe_state::call() const
{
    return get_action_child(CALL);
}

const nlhe_state* nlhe_state::raise(double fraction) const
{
    return static_cast<const nlhe_state*>(nlhe_state_base::raise(*this, fraction));
}

std::array<int, 2> nlhe_state::get_pot() const
{
    return pot_;
}

bool nlhe_state::is_raise(holdem_action action) const
{
    return action > CALL && action < ACTIONS;
}

bool nlhe_state::is_action_enabled(holdem_action action) const
{
    const auto mask = get_action_mask(action);
    return (enabled_actions_ & mask) == mask;
}

int nlhe_state::get_new_player_pot(const int player_pot, const int to_call, const int in_pot,
    const holdem_action action) const
{
    return nlhe_state_base::get_new_player_pot(player_pot, to_call, in_pot, action, stack_size_);
}

int nlhe_state::get_stack_size() const
{
    return stack_size_;
}

const nlhe_state* nlhe_state::get_action_child(int action) const
{
    for (int i = 0; i < children_.size(); ++i)
    {
        if (get_child(i)->get_action() == action)
            return get_child(i);
    }

    return nullptr;
}

std::unique_ptr<nlhe_state> nlhe_state::create(const std::string& config)
{
    static const boost::regex re("([^-]+)-([A-Za-z]+)-([0-9]+)");
    boost::smatch match;

    if (!boost::regex_match(config, match, re))
        throw std::runtime_error("unable to parse configuration");

    const auto& game = match[1].str();
    const auto& actions = match[2].str();
    const auto& stack = std::stoi(match[3].str());

    if (game == "nlhe")
    {
        int enabled_actions = 0;
        int limited_actions = 0;

        for (const auto c : actions)
        {
            switch (c)
            {
            case 'f': enabled_actions |= F_MASK; break;
            case 'c': enabled_actions |= C_MASK; break;
            case 'o': enabled_actions |= O_MASK; break;
            case 'h': enabled_actions |= H_MASK; break;
            case 'q': enabled_actions |= Q_MASK; break;
            case 'p': enabled_actions |= P_MASK; break;
            case 'w': enabled_actions |= W_MASK; break;
            case 'd': enabled_actions |= D_MASK; break;
            case 'v': enabled_actions |= V_MASK; break;
            case 't': enabled_actions |= T_MASK; break;
            case 'a': enabled_actions |= A_MASK; break;
            case 'F': enabled_actions |= F_MASK; limited_actions |= F_MASK; break;
            case 'C': enabled_actions |= C_MASK; limited_actions |= C_MASK; break;
            case 'O': enabled_actions |= O_MASK; limited_actions |= O_MASK; break;
            case 'H': enabled_actions |= H_MASK; limited_actions |= H_MASK; break;
            case 'Q': enabled_actions |= Q_MASK; limited_actions |= Q_MASK; break;
            case 'P': enabled_actions |= P_MASK; limited_actions |= P_MASK; break;
            case 'W': enabled_actions |= W_MASK; limited_actions |= W_MASK; break;
            case 'D': enabled_actions |= D_MASK; limited_actions |= D_MASK; break;
            case 'V': enabled_actions |= V_MASK; limited_actions |= V_MASK; break;
            case 'T': enabled_actions |= T_MASK; limited_actions |= T_MASK; break;
            case 'A': enabled_actions |= A_MASK; limited_actions |= A_MASK; break;
            default: throw std::runtime_error("unknown action");
            }
        }

        return std::unique_ptr<nlhe_state>(new nlhe_state(stack, enabled_actions, limited_actions));
    }

    throw std::runtime_error("unknown game configuration");
}
