#include <omp.h>
#include "util/binary_io.h"

template<class T, class U>
cs_cfr_solver<T, U>::cs_cfr_solver(std::unique_ptr<game_state> state, std::unique_ptr<abstraction_t> abstraction)
    : cfr_solver(std::move(state), std::move(abstraction))
{
}

template<class T, class U>
cs_cfr_solver<T, U>::~cs_cfr_solver()
{
}

template<class T, class U>
void cs_cfr_solver<T, U>::run_iteration(T& game)
{
    bucket_t buckets;
    const int result = game.play(&buckets);

    std::array<double, 2> reach = {{1.0, 1.0}};
    update(get_root_state(), buckets, reach, result);
}

template<class T, class U>
double cs_cfr_solver<T, U>::update(const game_state& state, const bucket_t& buckets, std::array<double, 2>& reach, const int result)
{
    const int player = state.get_player();
    const int opponent = player ^ 1;
    const int bucket = buckets[player][state.get_round()];
    std::array<double, ACTIONS> action_probabilities;

    assert(bucket >= 0 && bucket < get_abstraction().get_bucket_count(state.get_round()));

    get_regret_strategy(state, bucket, action_probabilities);

    if (reach[player] > EPSILON)
    {
        auto data = get_data(state.get_id(), bucket, 0);

        for (int i = 0; i < ACTIONS; ++i)
        {
            assert(state.get_child(i) || action_probabilities[i] == 0);

            // update average strategy
            if (action_probabilities[i] > EPSILON)
                data[i].strategy += reach[player] * action_probabilities[i];
        }
    }

    double total_ev = 0;
    std::array<double, ACTIONS> action_ev = {{}};
    const double old_reach = reach[player];

    for (int i = 0; i < ACTIONS; ++i)
    {
        // handle next state
        const game_state* next = state.get_child(i);

        if (!next)
            continue; // impossible action

        if (next->is_terminal())
        {
            action_ev[i] = next->get_terminal_ev(result);
        }
        else
        {
            reach[player] = old_reach * action_probabilities[i];

            if (reach[0] >= EPSILON || reach[1] >= EPSILON)
                action_ev[i] = update(*next, buckets, reach, result);
        }

        total_ev += action_probabilities[i] * action_ev[i];
    }

    reach[player] = old_reach;

    // update regrets
    if (reach[opponent] > EPSILON)
    {
        auto data = get_data(state.get_id(), bucket, 0);

        for (int i = 0; i < ACTIONS; ++i)
        {
            if (!state.get_child(i))
                continue;

            // counterfactual regret
            double delta_regret = (action_ev[i] - total_ev) * reach[opponent];

            if (player == 1)
                delta_regret = -delta_regret; // invert sign for P2

            data[i].regret += delta_regret;
        }
    }

    return total_ev;
}

template<class T, class U>
void cs_cfr_solver<T, U>::get_regret_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out) const
{
    const auto data = get_data(state.get_id(), bucket, 0);
    double bucket_sum = 0;

    for (int i = 0; i < ACTIONS; ++i)
    {
        assert(state.get_child(i) || data[i].regret <= EPSILON);

        if (data[i].regret > EPSILON)
            bucket_sum += data[i].regret;
    }

    if (bucket_sum > EPSILON)
    {
        for (int i = 0; i < ACTIONS; ++i)
            out[i] = data[i].regret > EPSILON ? data[i].regret / bucket_sum : 0;
    }
    else
    {
        for (int i = 0; i < ACTIONS; ++i)
            out[i] = state.get_child(i) ? 1.0 / state.get_child_count() : 0.0;
    }
}
