#include <omp.h>
#include "util/binary_io.h"

template<class T, class U>
pure_cfr_solver<T, U>::pure_cfr_solver(std::unique_ptr<game_state> state, std::unique_ptr<abstraction_t> abstraction)
    : cfr_solver(std::move(state), std::move(abstraction))
{
}

template<class T, class U>
pure_cfr_solver<T, U>::~pure_cfr_solver()
{
}

template<class T, class U>
void pure_cfr_solver<T, U>::run_iteration(T& game)
{
    bucket_t buckets;
    const int result = game.play(&buckets);

    update(0, game.get_random_engine(), get_root_state(), buckets, result);
    update(1, game.get_random_engine(), get_root_state(), buckets, result);
}

template<class T, class U>
typename pure_cfr_solver<T, U>::data_t pure_cfr_solver<T, U>::update(int position, std::mt19937& engine,
    const game_state& state, const bucket_t& buckets, const int result)
{
    const int player = state.get_player();
    const int bucket = buckets[player][state.get_round()];
    data_t total_ev = 0;

    assert(bucket >= 0 && bucket < get_abstraction().get_bucket_count(state.get_round()));

    const int choice = get_regret_strategy(engine, state, bucket);

    if (player != position)
    {
        auto data = get_data(state.get_id(), bucket, 0);

        assert(state.get_child(choice));

        // update average strategy
        if (data[choice].strategy == std::numeric_limits<data_t>::max())
            throw std::runtime_error("strategy data type overflow");

        ++data[choice].strategy;

        // handle next state
        const game_state* next = state.get_child(choice);

        assert(next);

        if (next->is_terminal())
            total_ev = next->get_terminal_ev(result);
        else
            total_ev = update(position, engine, *next, buckets, result);
    }
    else
    {
        std::array<int, ACTIONS> action_ev;

        for (int i = 0; i < ACTIONS; ++i)
        {
            const game_state* next = state.get_child(i);

            if (!next)
                continue; // impossible action

            if (next->is_terminal())
                action_ev[i] = next->get_terminal_ev(result);
            else
                action_ev[i] = update(position, engine, *next, buckets, result);
        }

        total_ev = action_ev[choice];

        // update regrets
        auto data = get_data(state.get_id(), bucket, 0);

        for (int i = 0; i < ACTIONS; ++i)
        {
            if (!state.get_child(i))
                continue;

            // counterfactual regret
            data_t delta_regret = action_ev[i] - total_ev;

            if (player == 1)
                delta_regret = -delta_regret; // invert sign for P2

            auto& regret = data[i].regret;

            // don't update regret if it would overflow (see open-pure-cfr)
            if ((delta_regret > 0 && regret > std::numeric_limits<data_t>::max() - delta_regret)
                || (delta_regret < 0 && regret < std::numeric_limits<data_t>::max() - delta_regret))
            {
                continue;
            }

            regret += delta_regret;
        }
    }

    return total_ev;
}

template<class T, class U>
int pure_cfr_solver<T, U>::get_regret_strategy(std::mt19937& engine, const game_state& state, const int bucket) const
{
    std::array<data_type, ACTIONS> data;

    for (int i = 0; i < ACTIONS; ++i)
        data[i] = *get_data(state.get_id(), bucket, i);

    std::uint64_t bucket_sum = 0;

    for (int i = 0; i < ACTIONS; ++i)
    {
        assert(state.get_child(i) || data[i].regret <= 0);

        if (data[i].regret > 0)
            bucket_sum += data[i].regret;
    }

    if (bucket_sum > 0)
    {
        // sample an action from a weighted distribution
        std::uniform_int_distribution<std::uint64_t> dist(0, bucket_sum);
        std::uint64_t x = dist(engine);

        for (int i = 0; i < ACTIONS; ++i)
        {
            const auto& regret = data[i].regret;

            if (regret <= 0)
                continue;

            if (x <= regret)
                return i;

            x -= regret;
        }

        assert(false);
        return -1;
    }
    else
    {
        if (state.get_child_count() == 0)
        {
            assert(false);
            return -1;
        }

        // sample an action from a uniform distribution
        std::uniform_int_distribution<int> dist(0, ACTIONS - 1);

        for (;;)
        {
            const int choice = dist(engine);

            if (state.get_child(choice))
                return choice;
        }
    }
}
