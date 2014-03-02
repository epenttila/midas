#include "nlhe_state.h"
#include <boost/integer/static_log2.hpp>
#include <random>
#include <boost/regex.hpp>

namespace
{
    static const std::array<int, 2> INITIAL_RAISE_MASKS = {{0, 0}};
    static const std::array<int, 2> INITIAL_POT = {{1, 2}};

    int soft_translate(const double b1, const double b, const double b2)
    {
        assert(b1 <= b && b <= b2);
        static std::random_device rd;
        static std::mt19937 engine(rd());
        static std::uniform_real_distribution<double> dist;
        const auto f = ((b2 - b) * (1 + b1)) / ((b2 - b1) * (1 + b));
        return dist(engine) < f ? 0 : 1;
    }

    int get_action_mask(nlhe_state::holdem_action action)
    {
        switch (action)
        {
        case nlhe_state::FOLD: return nlhe_state::F_MASK;
        case nlhe_state::CALL: return nlhe_state::C_MASK;
        case nlhe_state::RAISE_O: return nlhe_state::O_MASK;
        case nlhe_state::RAISE_H: return nlhe_state::H_MASK;
        case nlhe_state::RAISE_Q: return nlhe_state::Q_MASK;
        case nlhe_state::RAISE_P: return nlhe_state::P_MASK;
        case nlhe_state::RAISE_W: return nlhe_state::W_MASK;
        case nlhe_state::RAISE_D: return nlhe_state::D_MASK;
        case nlhe_state::RAISE_V: return nlhe_state::V_MASK;
        case nlhe_state::RAISE_T: return nlhe_state::T_MASK;
        case nlhe_state::RAISE_A: return nlhe_state::A_MASK;
        default: throw std::runtime_error("unknown action mask");
        }
    }
}

nlhe_state::nlhe_state(int stack_size, int enabled_actions, int limited_actions)
    : id_(0)
    , parent_(nullptr)
    , action_(INVALID_ACTION)
    , player_(0)
    , pot_(INITIAL_POT)
    , round_(PREFLOP)
    , stack_size_(stack_size)
    , raise_masks_(INITIAL_RAISE_MASKS)
    , limited_actions_(limited_actions)
    , enabled_actions_(enabled_actions)
{
    int id = id_ + 1;

    for (int i = 0; i < ACTIONS; ++i)
        create_child(static_cast<holdem_action>(i), &id);
}

nlhe_state::nlhe_state(const nlhe_state* parent, const holdem_action action_index, const int player,
    const std::array<int, 2>& pot, const game_round round, const std::array<int, 2> raise_masks, int enabled_actions,
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

    game_round new_round = round_;

    if (is_raise(prev_action) && next_action == CALL)
        new_round = static_cast<game_round>(new_round + 1); // raise-call
    else if (prev_action == CALL && next_action == CALL && parent_ && round_ == parent_->round_)
        new_round = static_cast<game_round>(new_round + 1); // check-check (or call-check preflop)

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

nlhe_state::game_round nlhe_state::get_round() const
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

    if (index >= children_.size())
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
    const auto& pot = get_pot();
    const auto player = get_player();

    if (get_stack_size() == pot[1 - player])
        return nullptr;

    std::array<double, ACTIONS> pot_sizes;

    for (int i = 0; i < static_cast<int>(pot_sizes.size()); ++i)
    {
        const auto p = get_action_child(static_cast<holdem_action>(i));

        if (p && i > CALL)
        {
            // don't use get_raise_factor here as we want to know the actual fractions wrt state::pot
            const auto player = get_player();
            const auto to_call = p->get_pot()[player] - p->get_pot()[1 - player];
            const auto in_pot = 2 * p->get_pot()[1 - player];
            pot_sizes[i] = static_cast<double>(to_call) / in_pot;
        }
        else
            pot_sizes[i] = -1;
    }

    holdem_action lower = INVALID_ACTION;
    holdem_action upper = INVALID_ACTION;

    for (std::size_t i = 0; i < pot_sizes.size(); ++i)
    {
        if (pot_sizes[i] <= fraction && (lower == INVALID_ACTION || pot_sizes[i] > pot_sizes[lower]))
            lower = static_cast<holdem_action>(i);
        else if (pot_sizes[i] >= fraction && (upper == INVALID_ACTION || pot_sizes[i] < pot_sizes[upper]))
            upper = static_cast<holdem_action>(i);
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

    return get_action_child(action);
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
    const auto factor = get_raise_factor(action);
    // round up to prune small bets
    return std::min(static_cast<int>(std::ceil(player_pot + to_call + (2 * to_call + in_pot) * factor)), stack_size_);
}

int nlhe_state::get_stack_size() const
{
    return stack_size_;
}

const nlhe_state* nlhe_state::get_action_child(int action) const
{
    for (const auto& p : children_)
    {
        if (p->get_action() == action)
            return p.get();
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

double nlhe_state::get_raise_factor(const holdem_action action)
{
    switch (action)
    {
    case RAISE_O: return 0.25;
    case RAISE_H: return 0.5;
    case RAISE_Q: return 0.75;
    case RAISE_P: return 1.0;
    case RAISE_W: return 1.5;
    case RAISE_D: return 2.0;
    case RAISE_V: return 5.0;
    case RAISE_T: return 10.0;
    case RAISE_A: return 999.0;
    default: throw std::runtime_error("unknown action raise factor");
    }
}

std::string nlhe_state::get_action_name(const holdem_action action)
{
    switch (action)
    {
    case FOLD: return "FOLD";
    case CALL: return "CALL";
    case RAISE_O: return "RAISE_O";
    case RAISE_H: return "RAISE_H";
    case RAISE_Q: return "RAISE_Q";
    case RAISE_P: return "RAISE_P";
    case RAISE_W: return "RAISE_W";
    case RAISE_D: return "RAISE_D";
    case RAISE_V: return "RAISE_V";
    case RAISE_T: return "RAISE_T";
    case RAISE_A: return "RAISE_A";
    default: throw std::runtime_error("unknown action name");
    }
}

std::ostream& operator<<(std::ostream& os, const nlhe_state& state)
{
    static const char actions[nlhe_state::ACTIONS + 1] = "fcohqpwdvta";
    std::string line;

    for (auto s = &state; s->get_parent() != nullptr; s = s->get_parent())
    {
        const char c = actions[s->get_action()];
        line = char(s->get_parent()->get_player() == 0 ? c : toupper(c)) + line;
    }

    os << state.get_id() << ":" << line;

    return os;
}
