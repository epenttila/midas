#include "nlhe_state_base.h"
#include <boost/regex.hpp>
#include "nlhe_state.h"

std::unique_ptr<nlhe_state_base> nlhe_state_base::create(const std::string& config)
{
    static const boost::regex re("([^-]+)-([a-z]+)-([0-9]+)");
    boost::smatch match;

    if (!boost::regex_match(config, match, re))
        throw std::runtime_error("unable to parse configuration");

    const auto& game = match[1].str();
    const auto& actions = match[2].str();
    const auto& stack = std::stoi(match[3].str());

    if (game == "nlhe")
    {
        if (actions == "fchqpwdta")
        {
            return std::unique_ptr<nlhe_state_base>(new nlhe_state<
                nlhe_state_base::F_MASK |
                nlhe_state_base::C_MASK |
                nlhe_state_base::H_MASK |
                nlhe_state_base::Q_MASK |
                nlhe_state_base::P_MASK |
                nlhe_state_base::W_MASK |
                nlhe_state_base::D_MASK |
                nlhe_state_base::T_MASK |
                nlhe_state_base::A_MASK>(stack));
        }
    }
    else if (game == "nlhe2")
    {
        if (actions == "fcohqpwdvta")
        {
            return std::unique_ptr<nlhe_state_base>(new nlhe_state<
                nlhe_state_base::F_MASK |
                nlhe_state_base::C_MASK |
                nlhe_state_base::O_MASK |
                nlhe_state_base::H_MASK |
                nlhe_state_base::Q_MASK |
                nlhe_state_base::P_MASK |
                nlhe_state_base::W_MASK |
                nlhe_state_base::D_MASK |
                nlhe_state_base::V_MASK |
                nlhe_state_base::T_MASK |
                nlhe_state_base::A_MASK>(stack, nlhe_state_base::O_MASK | nlhe_state_base::H_MASK | nlhe_state_base::Q_MASK));
        }
    }

    throw std::runtime_error("unknown game configuration");
}

int nlhe_state_base::get_action_mask(holdem_action action)
{
    switch (action)
    {
    case FOLD: return F_MASK;
    case CALL: return C_MASK;
    case RAISE_O: return O_MASK;
    case RAISE_H: return H_MASK;
    case RAISE_Q: return Q_MASK;
    case RAISE_P: return P_MASK;
    case RAISE_W: return W_MASK;
    case RAISE_D: return D_MASK;
    case RAISE_V: return V_MASK;
    case RAISE_T: return T_MASK;
    case RAISE_A: return A_MASK;
    default: throw std::runtime_error("unknown action mask");
    }
}

double nlhe_state_base::get_raise_factor(const holdem_action action)
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
    case RAISE_A: return ALLIN_BET_SIZE;
    default: throw std::runtime_error("unknown action raise factor");
    }
}

std::string nlhe_state_base::get_action_name(const holdem_action action)
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

int nlhe_state_base::soft_translate(const double b1, const double b, const double b2)
{
    assert(b1 <= b && b <= b2);
    static std::random_device rd;
    static std::mt19937 engine(rd());
    static std::uniform_real_distribution<double> dist;
    const auto f = ((b2 - b) * (1 + b1)) / ((b2 - b1) * (1 + b));
    return dist(engine) < f ? 0 : 1;
}

std::ostream& operator<<(std::ostream& os, const nlhe_state_base& state)
{
    static const char actions[nlhe_state_base::MAX_ACTIONS + 1] = "fcohqpwdvta";
    std::string line;

    for (auto s = &state; s->get_parent() != nullptr; s = s->get_parent())
    {
        const char c = actions[s->get_action(s->get_action_index())];
        line = char(s->get_parent()->get_player() == 0 ? c : toupper(c)) + line;
    }

    os << state.get_id() << ":" << line;

    return os;
}

const nlhe_state_base* nlhe_state_base::raise(const nlhe_state_base& state, double fraction)
{
    const auto stack_size = state.get_stack_size();
    const auto& pot = state.get_pot();
    const auto player = state.get_player();

    if (state.get_stack_size() == pot[1 - player])
        return nullptr;

    const auto to_call = pot[1 - player] - pot[player];
    const auto in_pot = pot[1 - player] + pot[player];

    std::array<double, MAX_ACTIONS> pot_sizes = {{
        -1,
        -1,
        get_raise_factor(RAISE_O),
        get_raise_factor(RAISE_H),
        get_raise_factor(RAISE_Q),
        get_raise_factor(RAISE_P),
        get_raise_factor(RAISE_W),
        get_raise_factor(RAISE_D),
        get_raise_factor(RAISE_V),
        get_raise_factor(RAISE_T),
        static_cast<double>(stack_size - to_call) / (to_call + in_pot), // to_call + x * (to_call + in_pot) = stack_size
    }};

    holdem_action lower = INVALID_ACTION;
    holdem_action upper = INVALID_ACTION;

    for (holdem_action i = static_cast<holdem_action>(CALL + 1); i < MAX_ACTIONS; i = static_cast<holdem_action>(i + 1))
    {
        const auto action_index = state.get_action_index(i);

        if (action_index == -1 || state.get_child(action_index) == nullptr)
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

    return state.get_child(state.get_action_index(action));
}

int nlhe_state_base::get_new_player_pot(const int player_pot, const int to_call, const int in_pot,
    const holdem_action action, int stack_size)
{
    const auto factor = get_raise_factor(action);
    return std::min(player_pot + int(to_call + (2 * to_call + in_pot) * factor), stack_size);
}
