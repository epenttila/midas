#include "nlhe_actor.h"
#include <cassert>
#include "cfrlib/nlhe_strategy.h"
#include "cfrlib/strategy.h"
#include "cfrlib/nl_holdem_state.h"
#include "nlhe_game.h"

nlhe_actor::nlhe_actor(const std::string& filename)
    : strategy_(new nlhe_strategy(filename))
{
}

void nlhe_actor::set_cards(int c0, int c1, int b0, int b1, int b2, int b3, int b4)
{
    strategy_->get_abstraction().get_buckets(c0, c1, b0, b1, b2, b3, b4, &buckets_);
    state_ = &strategy_->get_root_state();
}

void nlhe_actor::act(nlhe_game& g)
{
    /*std::stringstream ss;
    ss << *state_;
    auto  str = ss.str();
    str.empty();*/
    const int action_index = strategy_->get_strategy().get_action(state_->get_id(), buckets_[g.get_round()]);
    const int action = state_->get_action(action_index);
    //const double probability = strategy_info_.strategy->get(state_->get_id(), action_index, buckets_[g.get_round()]);
    const int prev_action_index = state_->get_action();
    const int prev_action = prev_action_index != -1 ? state_->get_action(prev_action_index) : -1;

    assert(state_->get_child(action_index));
    state_ = state_->get_child(action_index);
    //log_ << *state_ << "\n";
    //log_ << "Strategy: " << probability << "\n";

    switch (action)
    {
    case nlhe_state_base::FOLD:
        g.fold();
        break;
    case nlhe_state_base::CALL:
        if (prev_action == nlhe_state_base::RAISE_A)
            g.raise(99999);
        else
            g.call();
        break;
    default:
        {
            // TODO combine with main_window and nl_holdem_state
            const int to_call = g.get_to_call(id_);
            const int pot = g.get_pot(id_) - to_call;
            double factor;

            switch (action)
            {
            case nlhe_state_base::RAISE_H: factor = 0.5; break;
            case nlhe_state_base::RAISE_Q: factor = 0.75; break;
            case nlhe_state_base::RAISE_P: factor = 1.0; break;
            case nlhe_state_base::RAISE_W: factor = 1.5; break;
            case nlhe_state_base::RAISE_D: factor = 2.0; break;
            case nlhe_state_base::RAISE_T: factor = 10.0; break;
            case nlhe_state_base::RAISE_E: factor = 11.0; break;
            case nlhe_state_base::RAISE_A: factor = 99999.0; break;
            default: assert(false); factor = 1.0;
            }

            const int amount = g.get_bets(id_) + int(to_call + (2 * to_call + pot) * factor);
            g.raise(amount - g.get_bets(id_ ^ 1));
        }
        break;
    }
}

void nlhe_actor::opponent_called(const nlhe_game&)
{
    if (state_->is_terminal())
        return;

    assert(state_->call());
    state_ = state_->call();
}

void nlhe_actor::opponent_raised(const nlhe_game& g, int)
{
    /*std::stringstream ss;
    ss << *state_;
    auto  str = ss.str();
    str.empty();*/
    const int to_call = g.get_to_call(id_);
    const int pot = g.get_pot(id_);
    const double fraction = double(to_call) / pot;
    assert(fraction > 0.0);
    assert(state_->raise(fraction));
    state_ = state_->raise(fraction);
}
