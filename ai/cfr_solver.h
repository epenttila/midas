#pragma once

template<class game_state, int num_actions>
class cfr_solver : private boost::noncopyable
{
public:
    cfr_solver(const int num_states, const int num_buckets)
        : num_states_(num_states)
        , num_buckets_(num_buckets)
        , regrets_(new value[num_states_])
        , strategy_(new value[num_states_])
    {
        for (int i = 0; i < num_states; ++i)
        {
            regrets_[i] = new double[num_buckets][num_actions];
            strategy_[i] = new double[num_buckets][num_actions];

            for (int j = 0; j < num_buckets; ++j)
            {
                for (int k = 0; k < num_actions; ++k)
                {
                    regrets_[i][j][k] = 0;
                    strategy_[i][j][k] = 0;
                }
            }
        }
    }

    ~cfr_solver()
    {
        for (int i = 0; i < num_states_; ++i)
        {
            delete[] regrets_[i];
            delete[] strategy_[i];
        }

        delete[] regrets_;
        delete[] strategy_;
    }

    double update(const game_state& state, const std::array<int, 2>& buckets, const std::array<double, 2>& reach_probabilities)
    {
        const int current_player = state.get_player();
        std::array<double, num_actions> action_probabilities;
        get_regret_strategy(state, buckets[current_player], &action_probabilities[0]);
        double total_ev = 0.0;
        std::array<double, num_actions> action_ev;

        for (int i = 0; i < num_actions; ++i)
        {
            // update average strategy
            strategy_[state.get_id()][buckets[current_player]][i] += reach_probabilities[current_player] * action_probabilities[i];

            // handle next state
            game_state* next = state.get_child(i);

            if (next->is_terminal())
            {
                action_ev[i] = next->get_terminal_ev(buckets);
            }
            else
            {
                std::array<double, 2> new_reach;
                new_reach[current_player] = reach_probabilities[current_player] * action_probabilities[i];
                new_reach[1 - current_player] = reach_probabilities[1 - current_player];

                //if (new_reach[0] >= 1e-5 && new_reach[1] >= 1e-5)
                    action_ev[i] = update(*next, buckets, new_reach);
            }

            total_ev += action_probabilities[i] * action_ev[i];
        }

        // update regrets
        std::array<double, num_actions> delta_regrets;

        for (int i = 0; i < num_actions; ++i)
        {
            delta_regrets[i] = action_ev[i] - total_ev;
            delta_regrets[i] *= reach_probabilities[1 - current_player]; // counterfactual regret
            delta_regrets[i] *= current_player == 0 ? 1 : -1; // invert sign for P2
            regrets_[state.get_id()][buckets[current_player]][i] += delta_regrets[i];
        }

        return total_ev;
    }

    void get_regret_strategy(const game_state& state, const int bucket, double* out)
    {
        const auto& bucket_regret = regrets_[state.get_id()][bucket];
        const double default = 1.0 / num_actions;
        double bucket_sum = 0;

        for (int i = 0; i < num_actions; ++i)
            bucket_sum += std::max(0.0, bucket_regret[i]);

        for (int i = 0; i < num_actions; ++i)
        {
            if (bucket_sum > 0.0)
                *out++ = std::max(0.0, bucket_regret[i]) / bucket_sum;
            else
                *out++ = default;
        }
    }

    void get_average_strategy(const game_state& state, const int bucket, double* out)
    {
        const auto& bucket_strategy = strategy_[state.get_id()][bucket];
        const double default = 1.0 / num_actions;
        double bucket_sum = 0;

        for (int i = 0; i < num_actions; ++i)
            bucket_sum += bucket_strategy[i];

        for (int i = 0; i < num_actions; ++i)
        {
            if (bucket_sum > 0.0)
                *out++ = bucket_strategy[i] / bucket_sum;
            else
                *out++ = default;
        }
    }

    int get_num_buckets() const
    {
        return num_buckets_;
    }

private:
    const int num_states_;
    const int num_buckets_;
    typedef double (*value)[num_actions];
    value* regrets_;
    value* strategy_;
};
