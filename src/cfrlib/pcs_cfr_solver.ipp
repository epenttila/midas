//#include <omp.h>
#include "util/binary_io.h"

template<class T, class U>
pcs_cfr_solver<T, U>::pcs_cfr_solver(std::unique_ptr<game_state> state, std::unique_ptr<abstraction_t> abstraction)
    : base_t(std::move(state), std::move(abstraction))
{
}

template<class T, class U>
pcs_cfr_solver<T, U>::~pcs_cfr_solver()
{
}

template<class T, class U>
void pcs_cfr_solver<T, U>::run_iteration(T& game)
{
    buckets_type buckets;
    game.play_public(buckets);

    reach_type reach;
    reach[0].fill(1.0);
    reach[1].fill(1.0);

    ev_type utility = {{}};
    update(game, this->get_root_state(), buckets, reach, utility);
}

template<class T, class U>
void pcs_cfr_solver<T, U>::update(const game_type& game, const game_state& state, const buckets_type& buckets,
    const reach_type& reach, ev_type& utility)
{
    const int player = state.get_player();
    const int opponent = player ^ 1;

    // lookup infosets
    const typename buckets_type::value_type& round_buckets = buckets[state.get_round()];

    // regret matching & update average strategy
    std::array<std::array<double, PRIVATE>, ACTIONS> current_strategy = {{}};

    for (int infoset = 0; infoset < PRIVATE; ++infoset)
    {
        const int bucket = round_buckets[infoset];

        if (bucket == -1)
            continue;

        assert(bucket >= 0 && bucket < this->get_abstraction().get_bucket_count(state.get_round()));

        std::array<double, ACTIONS> s;
        get_regret_strategy(state, bucket, s);
        
        for (int action = 0; action < ACTIONS; ++action)
            current_strategy[action][infoset] = s[action];

        if (reach[player][infoset] > EPSILON)
        {
            auto data = this->get_data(state.get_id(), bucket, 0);

            for (int action = 0; action < ACTIONS; ++action)
            {
                assert(state.get_child(action) || current_strategy[action][infoset] == 0);

                if (current_strategy[action][infoset] > EPSILON)
                    data[action].strategy += reach[player][infoset] * current_strategy[action][infoset];
            }
        }
    }

    std::array<ev_type, ACTIONS> action_utility = {{}};

    for (int action = 0; action < ACTIONS; ++action)
    {
        const game_state* next = state.get_child(action);

        if (!next)
            continue;

        reach_type new_reach;
        std::array<double, 2> total_reach = {{0, 0}};

        for (int infoset = 0; infoset < PRIVATE; ++infoset)
        {
            new_reach[player][infoset] = reach[player][infoset] * current_strategy[action][infoset];
            new_reach[opponent][infoset] = reach[opponent][infoset];
            total_reach[player] += new_reach[player][infoset];
            total_reach[opponent] += new_reach[opponent][infoset];
        }

        if (total_reach[player] < EPSILON && total_reach[opponent] < EPSILON)
            continue;

        if (next->is_terminal())
        {
            // inline the update call for terminal nodes
            const int new_player = next->get_player();
            const int new_opponent = new_player ^ 1;

            std::array<typename T::results_type, 2> results = {{}};
            game.get_results(action, new_reach[new_opponent], results[new_player]);
            game.get_results(action, new_reach[new_player], results[new_opponent]);

            for (int infoset = 0; infoset < PRIVATE; ++infoset)
            {
                action_utility[action][new_player][infoset] = results[new_player][infoset] * next->get_terminal_ev(1);
                action_utility[action][new_opponent][infoset] = results[new_opponent][infoset] * next->get_terminal_ev(1);

                // TODO fix get_terminal_ev to return consistent values
                if (action == 0)
                    action_utility[action][1][infoset] = -action_utility[action][1][infoset];

                assert(action != 0 || (action_utility[action][new_player][infoset] >= 0 && action_utility[action][new_opponent][infoset] <= 0));
           }
        }
        else
        {
            update(game, *next, buckets, new_reach, action_utility[action]);
        }

        for (int infoset = 0; infoset < PRIVATE; ++infoset)
        {
            utility[player][infoset] += current_strategy[action][infoset] * action_utility[action][player][infoset];
            utility[opponent][infoset] += action_utility[action][opponent][infoset];
        }
    }

    // update regrets
    for (int infoset = 0; infoset < PRIVATE; ++infoset)
    {
        const int bucket = round_buckets[infoset];

        if (bucket == -1)
            continue;

        auto data = this->get_data(state.get_id(), bucket, 0);

        for (int action = 0; action < ACTIONS; ++action)
        {
            if (!state.get_child(action))
                continue;

            data[action].regret += action_utility[action][player][infoset] - utility[player][infoset];
        }
    }
}

template<class T, class U>
void pcs_cfr_solver<T, U>::get_regret_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out) const
{
    const auto data = this->get_data(state.get_id(), bucket, 0);
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
