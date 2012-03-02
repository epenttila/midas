#pragma once

namespace detail
{
    static const double epsilon = 1e-7;
}

class solver_base
{
    friend std::ostream& operator<<(std::ostream& o, const solver_base& solver);

public:
    virtual void solve(const int iterations) = 0;
    virtual void save_state(std::ostream&) const = 0;
    virtual void load_state(std::istream&) = 0;
    //virtual void save_strategy(std::ostream&) const = 0;
    virtual std::ostream& print(std::ostream&) const = 0;
};

inline std::ostream& operator<<(std::ostream& o, const solver_base& solver)
{
    return solver.print(o);
}

template<class game, int num_actions = game::state_t::ACTIONS, int num_rounds = game::state_t::ROUNDS>
class cfr_solver : public solver_base, private boost::noncopyable
{
public:
    typedef typename game::state_t game_state;
    typedef typename game::bucket_t bucket_t;
    typedef typename game::evaluator_t evaluator_t;
    typedef typename game::abstraction_t abstraction_t;
    typedef typename std::array<int, num_rounds> bucket_count_t;

    cfr_solver(const bucket_count_t& bucket_counts)
        : root_(new game_state)
        , evaluator_()
        , abstraction_(bucket_counts)
        , total_iterations_(0)
    {
        accumulated_regret_.fill(0);

        std::vector<const game_state*> stack(1, root_.get());

        while (!stack.empty())
        {
            const game_state* s = stack.back();
            stack.pop_back();

            if (!s->is_terminal())
            {
                assert(s->get_id() == int(states_.size()));
                states_.push_back(s);
            }

            for (int i = num_actions - 1; i >= 0; --i)
            {
                if (const game_state* child = s->get_child(i))
                    stack.push_back(child);
            }
        }

        assert(states_.size() > 1); // invalid game tree
        assert(root_->get_child_count() > 0);

        regrets_ = new value[states_.size()];
        strategy_ = new value[states_.size()];

        for (std::size_t i = 0; i < states_.size(); ++i)
        {
            if (states_[i]->is_terminal())
                continue;

            const int num_buckets = abstraction_.get_bucket_count(states_[i]->get_round());
            assert(num_buckets > 0);
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

    virtual void solve(const int iterations)
    {
        std::random_device rd;

        const double start_time = omp_get_wtime();
        double time = start_time;
        int iteration = 0; //iterations_;

#pragma omp parallel
        {
            game g;

#pragma omp for
            for (int i = 0; i < iterations; ++i)
            {
                bucket_t buckets;
                const int result = g.play(evaluator_, abstraction_, &buckets);

                std::array<double, 2> reach = {{1.0, 1.0}};
                update(*states_[0], buckets, reach, result);

#pragma omp atomic
                ++iteration;

                const double t = omp_get_wtime();

                if (iteration == iterations || (omp_get_thread_num() == 0 && t - time >= 1))
                {
                    std::array<double, 2> acfr;
                    acfr[0] = get_accumulated_regret(0) / (iteration + total_iterations_);
                    acfr[1] = get_accumulated_regret(1) / (iteration + total_iterations_);
                    const int ips = int(iteration / (t - start_time));
                    std::cout
                        << "iteration: " << iteration << " (" << ips << " i/s)"
                        << " regret: " << acfr[0] << ", " << acfr[1] << "\n";
                    time = t;
                }
            }
        }

        total_iterations_ += iteration;
    }

private:
    double update(const game_state& state, const bucket_t& buckets, std::array<double, 2>& reach, const int result)
    {
        const int player = state.get_player();
        const int opponent = player ^ 1;
        const int bucket = buckets[player][state.get_round()];
        std::array<double, num_actions> action_probabilities;

        assert(bucket >= 0 && bucket < abstraction_.get_bucket_count(state.get_round()));

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
                out[i] = state.get_child(i) ? 1.0 / state.get_child_count() : 0.0;
        }
    }

    void get_average_strategy(const game_state& state, const int bucket, std::array<double, num_actions>& out) const
    {
        const auto& bucket_strategy = strategy_[state.get_id()][bucket];
        double bucket_sum = 0;

        for (int i = 0; i < num_actions; ++i)
        {
            assert(state.get_child(i) || bucket_strategy[i] <= detail::epsilon);
            assert(bucket_strategy[i] >= 0);
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
                out[i] = state.get_child(i) ? 1.0 / state.get_child_count() : 0.0;
        }
    }

    double get_accumulated_regret(const int player) const
    {
        return accumulated_regret_[player];
    }

    void save_state(std::ostream& os) const
    {
        os.write(reinterpret_cast<const char*>(&total_iterations_), sizeof(int));
        os.write(reinterpret_cast<const char*>(&accumulated_regret_[0]), sizeof(double));
        os.write(reinterpret_cast<const char*>(&accumulated_regret_[1]), sizeof(double));

        for (std::size_t i = 0; i < states_.size(); ++i)
        {
            if (states_[i]->is_terminal())
                continue;

            for (int j = 0; j < abstraction_.get_bucket_count(states_[i]->get_round()); ++j)
            {
                for (int k = 0; k < num_actions; ++k)
                {
                    os.write(reinterpret_cast<char*>(&regrets_[i][j][k]), sizeof(double));
                    os.write(reinterpret_cast<char*>(&strategy_[i][j][k]), sizeof(double));
                }
            }
        }
    }

    void load_state(std::istream& is)
    {
        is.read(reinterpret_cast<char*>(&total_iterations_), sizeof(int));
        is.read(reinterpret_cast<char*>(&accumulated_regret_[0]), sizeof(double));
        is.read(reinterpret_cast<char*>(&accumulated_regret_[1]), sizeof(double));

        for (std::size_t i = 0; i < states_.size(); ++i)
        {
            if (states_[i]->is_terminal())
                continue;

            for (int j = 0; j < abstraction_.get_bucket_count(states_[i]->get_round()); ++j)
            {
                for (int k = 0; k < num_actions; ++k)
                {
                    is.read(reinterpret_cast<char*>(&regrets_[i][j][k]), sizeof(double));
                    is.read(reinterpret_cast<char*>(&strategy_[i][j][k]), sizeof(double));
                }
            }
        }
    }

    std::ostream& print(std::ostream& os) const
    {
        os << "total iterations: " << total_iterations_ << "\n";
        os << "internal states: " << states_.size() << "\n";
        os << "accumulated regret: " << accumulated_regret_[0] << ", " << accumulated_regret_[1] << "\n";

        for (std::size_t i = 0; i < states_.size(); ++i)
        {
            if (states_[i]->is_terminal())
                continue;

            std::string line;

            for (int j = 0; j < abstraction_.get_bucket_count(states_[i]->get_round()); ++j)
            {
                os << *states_[i] << "," << j;

                std::array<double, num_actions> p;
                get_average_strategy(*states_[i], j, p);

                for (std::size_t k = 0; k < p.size(); ++k)
                    os << "," << std::fixed << p[k];

                os << "\n";
            }
        }

        return os;
    }

private:
    std::vector<const game_state*> states_;
    typedef double (*value)[num_actions];
    value* regrets_;
    value* strategy_;
    std::array<double, 2> accumulated_regret_;
    std::unique_ptr<game_state> root_;
    const evaluator_t evaluator_;
    const abstraction_t abstraction_;
    int total_iterations_;
};
