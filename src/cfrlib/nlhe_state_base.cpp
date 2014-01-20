#include "nlhe_state_base.h"
#include <boost/regex.hpp>
#include "nlhe_state.h"
#include "nlhe_state_v2.h"

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
            return std::unique_ptr<nlhe_state_base>(new nlhe_state_v2<
                nlhe_state_base::O_MASK |
                nlhe_state_base::F_MASK |
                nlhe_state_base::C_MASK |
                nlhe_state_base::H_MASK |
                nlhe_state_base::Q_MASK |
                nlhe_state_base::P_MASK |
                nlhe_state_base::W_MASK |
                nlhe_state_base::D_MASK |
                nlhe_state_base::V_MASK |
                nlhe_state_base::T_MASK |
                nlhe_state_base::A_MASK>(stack));
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
    const double s1 = (b1 / b - b1 / b2) / (1 - b1 / b2);
    const double s2 = (b / b2 - b1 / b2) / (1 - b1 / b2);
    return dist(engine) < (s1 / (s1 + s2)) ? 0 : 1;
}

std::ostream& operator<<(std::ostream& os, const nlhe_state_base& state)
{
    state.print(os);
    return os;
}
