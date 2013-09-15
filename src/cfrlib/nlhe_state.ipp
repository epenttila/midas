#include <boost/integer/static_log2.hpp>
#include <random>
#include "util/game.h"
#include "holdem_game.h"

namespace detail
{
    static const std::array<int, 2> INITIAL_POT = {{1, 2}};

    int soft_translate(const double b1, const double b, const double b2);

    template<unsigned int N>
    struct count_bits
    {
        static const unsigned int value = count_bits<(N >> 1)>::value + (N % 2);
    };

    template<>
    struct count_bits<0>
    {
        static const unsigned int value = 0;
    };
}

template<int BITMASK>
nlhe_state<BITMASK>::nlhe_state(const int stack_size)
    : id_(0)
    , parent_(nullptr)
    , action_index_(-1)
    , player_(0)
    , pot_(detail::INITIAL_POT)
    , round_(PREFLOP)
    , child_count_(0) // sb has all actions
    , stack_size_(stack_size)
{
    children_.fill(nullptr);

    for (int i = 0, j = 0; i < MAX_ACTIONS; ++i)
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
nlhe_state<BITMASK>::nlhe_state(const nlhe_state* parent, const int action_index, const int player,
    const std::array<int, 2>& pot, const int round, int* id)
    : id_(id ? (*id)++ : -1)
    , parent_(parent)
    , action_index_(action_index)
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

template<int BITMASK>
nlhe_state<BITMASK>::~nlhe_state()
{
    std::for_each(children_.begin(), children_.end(), [](nlhe_state* p) { delete p; });
}

template<int BITMASK>
void nlhe_state<BITMASK>::create_child(const int action_index, int* id)
{
    if (is_terminal())
        return;

    const bool opponent_allin = pot_[1 - player_] == stack_size_;
    const int next_action = index_to_action_[action_index];
    const int prev_action = action_index_ == -1 ? -1 : index_to_action_[action_index_];

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

    std::array<int, 2> new_pot = {{pot_[0], pot_[1]}};

    const int to_call = pot_[1 - player_] - pot_[player_];
    const int in_pot = pot_[1 - player_] + pot_[player_] - to_call;

    assert(pot_[1 - player_] == stack_size_
        || prev_action == -1
        || prev_action == FOLD
        || prev_action == CALL
        || (prev_action == RAISE_H && to_call == int(0.5 * in_pot))
        || (prev_action == RAISE_Q && to_call == int(0.75 * in_pot))
        || (prev_action == RAISE_P && to_call == in_pot)
        || (prev_action == RAISE_W && to_call == int(1.5 * in_pot))
        || (prev_action == RAISE_D && to_call == 2 * in_pot)
        || (prev_action == RAISE_T && to_call == 10 * in_pot)
        || (prev_action == RAISE_E && to_call == 11 * in_pot));

    switch (next_action)
    {
    case FOLD:
        break;
    case CALL:
        new_pot[player_] = pot_[1 - player_];
        break;
    default:
        {
            const int new_player_pot = get_new_player_pot(pot_[player_], to_call, in_pot, next_action);
            const int max_player_pot = get_new_player_pot(pot_[player_], to_call, in_pot, get_max_action());

            if (new_player_pot >= max_player_pot && next_action != get_max_action())
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

    children_[action_index] = new nlhe_state(this, action_index, new_player, new_pot, new_round,
        new_terminal ? nullptr : id);

    ++child_count_;
}

template<int BITMASK>
int nlhe_state<BITMASK>::get_terminal_ev(const int result) const
{
    const int action = index_to_action_[action_index_];
    assert(is_terminal());
    assert(action != CALL || pot_[0] == pot_[1]);

    if (action == FOLD)
        return parent_->player_ == 1 ? pot_[1] : -pot_[0];
    else if (action == CALL)
        return result * pot_[0];

    assert(false);
    return 0;
}

template<int BITMASK>
int nlhe_state<BITMASK>::get_action() const
{
    return action_index_;
}

template<int BITMASK>
int nlhe_state<BITMASK>::get_round() const
{
    return round_;
}

template<int BITMASK>
const nlhe_state<BITMASK>* nlhe_state<BITMASK>::get_parent() const
{
    return parent_;
}

template<int BITMASK>
bool nlhe_state<BITMASK>::is_terminal() const
{
    return id_ == -1;
}

template<int BITMASK>
const nlhe_state<BITMASK>* nlhe_state<BITMASK>::get_child(int index) const
{
    return children_[index];
}

template<int BITMASK>
int nlhe_state<BITMASK>::get_id() const
{
    return id_;
}

template<int BITMASK>
int nlhe_state<BITMASK>::get_player() const
{
    return player_;
}

template<int BITMASK>
int nlhe_state<BITMASK>::get_child_count() const
{
    return child_count_;
}

template<int BITMASK>
void nlhe_state<BITMASK>::print(std::ostream& os) const
{
    static const char actions[nlhe_state_base::MAX_ACTIONS] = {'f', 'c', 'h', 'q', 'p', 'w', 'd', 't', 'e', 'a'};
    std::string line;

    for (const nlhe_state<BITMASK>* s = this; s->get_parent() != nullptr; s = s->get_parent())
    {
        const char c = actions[index_to_action_[s->get_action()]];
        line = char(s->get_parent()->get_player() == 0 ? c : toupper(c)) + line;
    }

    os << get_id() << ":" << line;
}

template<int BITMASK>
const nlhe_state<BITMASK>* nlhe_state<BITMASK>::call() const
{
    return children_[action_to_index_[CALL]];
}

template<int BITMASK>
const nlhe_state<BITMASK>* nlhe_state<BITMASK>::raise(double fraction) const
{
    if (stack_size_ == pot_[1 - player_])
        return nullptr;

    std::array<double, MAX_ACTIONS> pot_sizes = {{
        -1,
        -1,
        0.5,
        0.75,
        1.0,
        1.5,
        2.0,
        10.0,
        11.0,
        (stack_size_ - pot_[1 - player_]) / double(2 * pot_[1 - player_]),
    }};

    int lower = -1;
    int upper = -1;

    for (int i = CALL + 1; i < MAX_ACTIONS; ++i)
    {
        if (!is_action_enabled(i) || children_[action_to_index_[i]] == nullptr)
            continue;

        if (pot_sizes[i] <= fraction && (lower == -1 || pot_sizes[i] > pot_sizes[lower]))
            lower = i;
        else if (pot_sizes[i] >= fraction && (upper == -1 || pot_sizes[i] < pot_sizes[upper]))
            upper = i;
    }

    assert(lower != -1 || upper != -1);

    int action;

    if (lower == -1)
        action = upper;
    else if (upper == -1)
        action = lower;
    else if (lower == upper)
        action = lower;
    else
        action = detail::soft_translate(pot_sizes[lower], fraction, pot_sizes[upper]) == 0 ? lower : upper;

    return children_[action_to_index_[action]];
}

template<int BITMASK>
std::array<int, 2> nlhe_state<BITMASK>::get_pot() const
{
    return pot_;
}

template<int BITMASK>
int nlhe_state<BITMASK>::get_action_count() const
{
    return ACTIONS;
}

template<int BITMASK>
int nlhe_state<BITMASK>::get_action(int index) const
{
    return index_to_action_[index];
}

template<int BITMASK>
bool nlhe_state<BITMASK>::is_raise(int action) const
{
    return action > CALL && action < MAX_ACTIONS;
}

template<int BITMASK>
bool nlhe_state<BITMASK>::is_action_enabled(int action) const
{
    switch (action)
    {
    case FOLD: return (BITMASK & F_MASK) == F_MASK;
    case CALL: return (BITMASK & C_MASK) == C_MASK;
    case RAISE_H: return (BITMASK & H_MASK) == H_MASK;
    case RAISE_Q: return (BITMASK & Q_MASK) == Q_MASK;
    case RAISE_P: return (BITMASK & P_MASK) == P_MASK;
    case RAISE_W: return (BITMASK & W_MASK) == W_MASK;
    case RAISE_D: return (BITMASK & D_MASK) == D_MASK;
    case RAISE_T: return (BITMASK & T_MASK) == T_MASK;
    case RAISE_E: return (BITMASK & E_MASK) == E_MASK;
    case RAISE_A: return (BITMASK & A_MASK) == A_MASK;
    }

    return false;
}

template<int BITMASK>
int nlhe_state<BITMASK>::get_max_action() const
{
    return boost::static_log2<BITMASK>::value;
}

template<int BITMASK>
int nlhe_state<BITMASK>::get_new_player_pot(const int player_pot, const int to_call, const int in_pot, const int action) const
{
    const auto factor = get_raise_factor(action);
    return std::min(player_pot + int(to_call + (2 * to_call + in_pot) * factor), stack_size_);
}

template<int BITMASK>
double nlhe_state<BITMASK>::get_raise_factor(const int action) const
{
    switch (action)
    {
    case RAISE_H: return 0.5;
    case RAISE_Q: return 0.75;
    case RAISE_P: return 1.0;
    case RAISE_W: return 1.5;
    case RAISE_D: return 2.0;
    case RAISE_T: return 10.0;
    case RAISE_E: return 11.0;
    case RAISE_A: return ALLIN_BET_SIZE;
    default: assert(false); return 1.0;
    }
}

template<int BITMASK>
std::array<int, nlhe_state<BITMASK>::MAX_ACTIONS> nlhe_state<BITMASK>::action_to_index_;

template<int BITMASK>
std::array<int, nlhe_state<BITMASK>::ACTIONS> nlhe_state<BITMASK>::index_to_action_;
