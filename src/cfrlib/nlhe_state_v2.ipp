#include <boost/integer/static_log2.hpp>
#include <random>
#include <boost/algorithm/clamp.hpp>
#include "util/game.h"
#include "holdem_game.h"

namespace detail
{
    static const std::array<int, 2> INITIAL_RAISE_MASKS = {{0, 0}};
}

template<int BITMASK>
nlhe_state_v2<BITMASK>::nlhe_state_v2(const int stack_size)
    : id_(0)
    , parent_(nullptr)
    , action_index_(-1)
    , player_(0)
    , pot_(detail::INITIAL_POT)
    , round_(PREFLOP)
    , child_count_(0) // sb has all actions
    , stack_size_(stack_size)
    , raise_masks_(detail::INITIAL_RAISE_MASKS)
{
    children_.fill(nullptr);

    int j = 0;

    for (holdem_action i = FOLD; i < MAX_ACTIONS; i = static_cast<holdem_action>(i + 1))
    {
        if (is_action_enabled(i))
        {
            action_to_index_[i] = j;
            index_to_action_[j++] = i;
        }
        else
        {
            action_to_index_[i] = -1;
        }
    }

    int id = id_ + 1;

    for (int i = 0; i < ACTIONS; ++i)
        create_child(i, &id);
}

template<int BITMASK>
nlhe_state_v2<BITMASK>::nlhe_state_v2(const nlhe_state_v2* parent, const int action_index, const int player,
    const std::array<int, 2>& pot, const int round, const std::array<int, 2> raise_masks, int* id)
    : id_(id ? (*id)++ : -1)
    , parent_(parent)
    , action_index_(action_index)
    , player_(player)
    , pot_(pot)
    , round_(round)
    , child_count_(0)
    , stack_size_(parent->stack_size_)
    , raise_masks_(raise_masks)
{
    children_.fill(nullptr);

    for (int i = 0; i < ACTIONS; ++i)
        create_child(i, id);
}

template<int BITMASK>
nlhe_state_v2<BITMASK>::~nlhe_state_v2()
{
    std::for_each(children_.begin(), children_.end(), [](nlhe_state_v2* p) { delete p; });
}

template<int BITMASK>
void nlhe_state_v2<BITMASK>::create_child(const int action_index, int* id)
{
    if (is_terminal())
        return;

    const bool opponent_allin = pot_[1 - player_] == stack_size_;
    const holdem_action next_action = index_to_action_[action_index];
    const holdem_action prev_action = action_index_ == -1 ? INVALID_ACTION : index_to_action_[action_index_];

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

        if ((new_raise_masks[player_] & mask) == mask)
            return;
        else
            new_raise_masks[player_] |= mask;
    }

    std::array<int, 2> new_pot = {{pot_[0], pot_[1]}};

    const int to_call = pot_[1 - player_] - pot_[player_];
    const int in_pot = pot_[1 - player_] + pot_[player_] - to_call;

    assert(pot_[1 - player_] == stack_size_
        || prev_action == -1
        || prev_action == FOLD
        || prev_action == CALL
        || (prev_action == RAISE_O && to_call == std::max(to_call == 0 ? 2 : to_call, int(0.25 * in_pot)))
        || (prev_action == RAISE_H && to_call == int(0.5 * in_pot))
        || (prev_action == RAISE_Q && to_call == int(0.75 * in_pot))
        || (prev_action == RAISE_P && to_call == in_pot)
        || (prev_action == RAISE_W && to_call == int(1.5 * in_pot))
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

    assert(children_[action_index] == nullptr);

    children_[action_index] = new nlhe_state_v2(this, action_index, new_player, new_pot, new_round, new_raise_masks,
        new_terminal ? nullptr : id);

    ++child_count_;
}

template<int BITMASK>
int nlhe_state_v2<BITMASK>::get_terminal_ev(const int result) const
{
    const int action = index_to_action_[action_index_];
    assert(is_terminal());
    assert(action != CALL || pot_[0] == pot_[1]);

    if (action == FOLD)
    {
        return parent_->player_ == 1 ? pot_[1] : -pot_[0];
    }
    else if (action == CALL)
    {
        assert(pot_[0] == pot_[1]);
        return result * pot_[0];
    }

    assert(false);
    return 0;
}

template<int BITMASK>
int nlhe_state_v2<BITMASK>::get_action_index() const
{
    return action_index_;
}

template<int BITMASK>
int nlhe_state_v2<BITMASK>::get_round() const
{
    return round_;
}

template<int BITMASK>
const nlhe_state_v2<BITMASK>* nlhe_state_v2<BITMASK>::get_parent() const
{
    return parent_;
}

template<int BITMASK>
bool nlhe_state_v2<BITMASK>::is_terminal() const
{
    return id_ == -1;
}

template<int BITMASK>
const nlhe_state_v2<BITMASK>* nlhe_state_v2<BITMASK>::get_child(int index) const
{
    return children_[index];
}

template<int BITMASK>
int nlhe_state_v2<BITMASK>::get_id() const
{
    return id_;
}

template<int BITMASK>
int nlhe_state_v2<BITMASK>::get_player() const
{
    return player_;
}

template<int BITMASK>
int nlhe_state_v2<BITMASK>::get_child_count() const
{
    return child_count_;
}

template<int BITMASK>
void nlhe_state_v2<BITMASK>::print(std::ostream& os) const
{
    static const char actions[nlhe_state_base::MAX_ACTIONS + 1] = "fcohqpwdvta";
    std::string line;

    for (const nlhe_state_v2<BITMASK>* s = this; s->get_parent() != nullptr; s = s->get_parent())
    {
        const char c = actions[index_to_action_[s->get_action_index()]];
        line = char(s->get_parent()->get_player() == 0 ? c : toupper(c)) + line;
    }

    os << get_id() << ":" << line;
}

template<int BITMASK>
const nlhe_state_v2<BITMASK>* nlhe_state_v2<BITMASK>::call() const
{
    return children_[action_to_index_[CALL]];
}

template<int BITMASK>
const nlhe_state_v2<BITMASK>* nlhe_state_v2<BITMASK>::raise(double fraction) const
{
    if (stack_size_ == pot_[1 - player_])
        return nullptr;

    std::array<double, MAX_ACTIONS> pot_sizes = {{
        -1, // FOLD
        -1, // CALL
        get_raise_factor(RAISE_O),
        get_raise_factor(RAISE_H),
        get_raise_factor(RAISE_Q),
        get_raise_factor(RAISE_P),
        get_raise_factor(RAISE_W),
        get_raise_factor(RAISE_D),
        get_raise_factor(RAISE_V),
        get_raise_factor(RAISE_T),
        (stack_size_ - pot_[1 - player_]) / double(2 * pot_[1 - player_]),
    }};

    holdem_action lower = INVALID_ACTION;
    holdem_action upper = INVALID_ACTION;

    for (holdem_action i = static_cast<holdem_action>(CALL + 1); i < MAX_ACTIONS; i = static_cast<holdem_action>(i + 1))
    {
        if (!is_action_enabled(i) || children_[action_to_index_[i]] == nullptr)
            continue;

        if (pot_sizes[i] <= fraction && (lower == INVALID_ACTION || pot_sizes[i] > pot_sizes[lower]))
            lower = i;
        else if (pot_sizes[i] >= fraction && (upper == INVALID_ACTION || pot_sizes[i] < pot_sizes[upper]))
            upper = i;
    }

    assert(lower != INVALID_ACTION || upper != INVALID_ACTION);

    holdem_action action;

    if (lower == INVALID_ACTION)
        action = upper;
    else if (upper == INVALID_ACTION)
        action = lower;
    else if (lower == upper)
        action = lower;
    else
        action = soft_translate(pot_sizes[lower], fraction, pot_sizes[upper]) == 0 ? lower : upper;

    return children_[action_to_index_[action]];
}

template<int BITMASK>
std::array<int, 2> nlhe_state_v2<BITMASK>::get_pot() const
{
    return pot_;
}

template<int BITMASK>
int nlhe_state_v2<BITMASK>::get_action_count() const
{
    return ACTIONS;
}

template<int BITMASK>
typename nlhe_state_v2<BITMASK>::holdem_action nlhe_state_v2<BITMASK>::get_action(int index) const
{
    return index_to_action_[index];
}

template<int BITMASK>
bool nlhe_state_v2<BITMASK>::is_raise(holdem_action action) const
{
    return action > CALL && action < MAX_ACTIONS;
}

template<int BITMASK>
bool nlhe_state_v2<BITMASK>::is_action_enabled(holdem_action action) const
{
    const auto mask = get_action_mask(action);
    return (BITMASK & mask) == mask;
}

template<int BITMASK>
typename nlhe_state_v2<BITMASK>::holdem_action nlhe_state_v2<BITMASK>::get_max_action() const
{
    return static_cast<holdem_action>(boost::static_log2<BITMASK>::value);
}

template<int BITMASK>
int nlhe_state_v2<BITMASK>::get_new_player_pot(const int player_pot, const int to_call, const int in_pot,
    const holdem_action action) const
{
    const auto factor = get_raise_factor(action);
    return std::min(player_pot + int(to_call + (2 * to_call + in_pot) * factor), stack_size_);
}

template<int BITMASK>
int nlhe_state_v2<BITMASK>::get_stack_size() const
{
    return stack_size_;
}

template<int BITMASK>
std::array<int, nlhe_state_v2<BITMASK>::MAX_ACTIONS> nlhe_state_v2<BITMASK>::action_to_index_;

template<int BITMASK>
std::array<typename nlhe_state_v2<BITMASK>::holdem_action, nlhe_state_v2<BITMASK>::ACTIONS> nlhe_state_v2<BITMASK>::index_to_action_;
