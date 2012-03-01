#pragma once

namespace detail
{
    static const double epsilon = 1e-7;
}

template<class game_state, int num_actions, int num_rounds>
class cfr_solver : private boost::noncopyable
{
public:
    typedef std::array<int, num_rounds> bucket_count_t;
    typedef std::array<std::array<int, num_rounds>, 2> bucket_t;

    cfr_solver(const std::vector<game_state*>& states, const bucket_count_t& bucket_counts)
        : states_(states)
        , bucket_counts_(bucket_counts)
        , regrets_(new value[states.size()])
        , strategy_(new value[states.size()])
    {
        accumulated_regret_.fill(0);

        for (std::size_t i = 0; i < states.size(); ++i)
        {
            if (states[i]->is_terminal())
                continue;

            const int num_buckets = bucket_counts[states[i]->get_round()];
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
        for (std::size_t i = 0; i < states_.size(); ++i)
        {
            delete[] regrets_[i];
            delete[] strategy_[i];
        }

        delete[] regrets_;
        delete[] strategy_;
    }

    double update(const game_state& state, const bucket_t& buckets, std::array<double, 2>& reach, const int result)
    {
        const int player = state.get_player();
        const int opponent = player ^ 1;
        const int bucket = buckets[player][state.get_round()];
        std::array<double, num_actions> action_probabilities;

        get_regret_strategy(state, bucket, action_probabilities);

        if (reach[player] > detail::epsilon)
        {
            auto& strategy = strategy_[state.get_id()][bucket];

            for (int i = 0; i < num_actions; ++i)
            {
                assert(state.get_child(i) || action_probabilities[i] == 0);

                // update average strategy
                if (action_probabilities[i] > detail::epsilon)
                    strategy[i] += reach[player] * action_probabilities[i];
            }
        }

        double total_ev = 0;
        std::array<double, num_actions> action_ev;
        const double old_reach = reach[player];

        for (int i = 0; i < num_actions; ++i)
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

                if (reach[0] >= detail::epsilon || reach[1] >= detail::epsilon)
                    action_ev[i] = update(*next, buckets, reach, result);
                else
                    action_ev[i] = 0;
            }

            total_ev += action_probabilities[i] * action_ev[i];
        }

        reach[player] = old_reach;

        // update regrets
        if (reach[opponent] > detail::epsilon)
        {
            auto& regrets = regrets_[state.get_id()][bucket];

            for (int i = 0; i < num_actions; ++i)
            {
                if (!state.get_child(i))
                    continue;

                // counterfactual regret
                const double delta_regret = (action_ev[i] - total_ev) * reach[opponent];

                regrets[i] += player == 0 ? delta_regret : -delta_regret; // invert sign for P2

                if (delta_regret > detail::epsilon)
                    accumulated_regret_[player] += delta_regret;
            }
        }

        return total_ev;
    }

    void get_regret_strategy(const game_state& state, const int bucket, std::array<double, num_actions>& out)
    {
        const auto& bucket_regret = regrets_[state.get_id()][bucket];
        double bucket_sum = 0;

        for (int i = 0; i < num_actions; ++i)
        {
            assert(state.get_child(i) || bucket_regret[i] <= detail::epsilon);

            if (bucket_regret[i] > detail::epsilon)
                bucket_sum += bucket_regret[i];
        }

        if (bucket_sum > detail::epsilon)
        {
            for (int i = 0; i < num_actions; ++i)
                out[i] = bucket_regret[i] > detail::epsilon ? bucket_regret[i] / bucket_sum : 0;
        }
        else
        {
            for (int i = 0; i < num_actions; ++i)
                out[i] = state.get_child(i) ? 1.0 / state.get_num_actions() : 0.0;
        }
    }

    void get_average_strategy(const game_state& state, const int bucket, std::array<double, num_actions>& out) const
    {
        const auto& bucket_strategy = strategy_[state.get_id()][bucket];
        double bucket_sum = 0;

        for (int i = 0; i < num_actions; ++i)
        {
            assert(state.get_child(i) || bucket_strategy[i] <= detail::epsilon);
            bucket_sum += bucket_strategy[i];
        }

        if (bucket_sum > detail::epsilon)
        {
            for (int i = 0; i < num_actions; ++i)
                out[i] = bucket_strategy[i] / bucket_sum;
        }
        else
        {
            for (int i = 0; i < num_actions; ++i)
                out[i] = state.get_child(i) ? 1.0 / state.get_num_actions() : 0.0;
        }
    }

    int get_bucket_count(const int round) const
    {
        return bucket_counts_[round];
    }

    double get_accumulated_regret(const int player) const
    {
        return accumulated_regret_[player];
    }

    void save(std::ostream& os) const
    {
        os.write(reinterpret_cast<const char*>(&accumulated_regret_[0]), sizeof(double));
        os.write(reinterpret_cast<const char*>(&accumulated_regret_[1]), sizeof(double));

        for (std::size_t i = 0; i < states_.size(); ++i)
        {
            if (states_[i]->is_terminal())
                continue;

            for (int j = 0; j < bucket_counts_[states_[i]->get_round()]; ++j)
            {
                for (int k = 0; k < num_actions; ++k)
                {
                    os.write(reinterpret_cast<char*>(&regrets_[i][j][k]), sizeof(double));
                    os.write(reinterpret_cast<char*>(&strategy_[i][j][k]), sizeof(double));
                }
            }
        }
    }

    void load(std::istream& is)
    {
        is.read(reinterpret_cast<char*>(&accumulated_regret_[0]), sizeof(double));
        is.read(reinterpret_cast<char*>(&accumulated_regret_[1]), sizeof(double));

        for (std::size_t i = 0; i < states_.size(); ++i)
        {
            if (states_[i]->is_terminal())
                continue;

            for (int j = 0; j < bucket_counts_[states_[i]->get_round()]; ++j)
            {
                for (int k = 0; k < num_actions; ++k)
                {
                    is.read(reinterpret_cast<char*>(&regrets_[i][j][k]), sizeof(double));
                    is.read(reinterpret_cast<char*>(&strategy_[i][j][k]), sizeof(double));
                }
            }
        }
    }

private:
    const std::vector<game_state*> states_;
    const bucket_count_t bucket_counts_;
    typedef double (*value)[num_actions];
    value* regrets_;
    value* strategy_;
    std::array<double, 2> accumulated_regret_;
};
