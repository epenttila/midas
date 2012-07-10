#include "cfr_solver.h"
#include <cassert>
#include <iostream>
#include <omp.h>
#include <boost/format.hpp>

namespace
{
    static const double EPSILON = 1e-7;
}

template<class T, class U>
cfr_solver<T, U>::cfr_solver(const std::string& bucket_configuration, const int stack_size)
    : root_(new game_state(stack_size))
    , evaluator_()
    , abstraction_(bucket_configuration)
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

        for (int i = ACTIONS - 1; i >= 0; --i)
        {
            if (const game_state* child = s->get_child(i))
                stack.push_back(child);
        }
    }

    std::cout << states_.size() << " game states\n";

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
        regrets_[i] = new double[num_buckets][ACTIONS];
        strategy_[i] = new double[num_buckets][ACTIONS];

        for (int j = 0; j < num_buckets; ++j)
        {
            for (int k = 0; k < ACTIONS; ++k)
            {
                regrets_[i][j][k] = 0;
                strategy_[i][j][k] = 0;
            }
        }
    }
}

template<class T, class U>
cfr_solver<T, U>::~cfr_solver()
{
    for (std::size_t i = 0; i < states_.size(); ++i)
    {
        delete[] regrets_[i];
        delete[] strategy_[i];
    }

    delete[] regrets_;
    delete[] strategy_;
}

template<class T, class U>
void cfr_solver<T, U>::solve(const int iterations)
{
    const double start_time = omp_get_wtime();
    double time = start_time;
    int iteration = 0;

#pragma omp parallel
    {
        T g;

#pragma omp for schedule(dynamic)
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
                const double duration = t - start_time;
                const int hour = int(duration / 3600);
                const int minute = int(duration / 60 - hour * 60);
                const int second = int(duration - minute * 60 - hour * 3600);
                const int ips = int(iteration / duration);
                std::array<double, 2> acfr;
                acfr[0] = get_accumulated_regret(0) / (iteration + total_iterations_);
                acfr[1] = get_accumulated_regret(1) / (iteration + total_iterations_);
                std::cout << boost::format("%02d:%02d:%02d: %d (%d i/s) ACFR: %f, %f\n") %
                    hour % minute % second % iteration % ips % acfr[0] % acfr[1];
                time = t;
            }
        }
    }

    total_iterations_ += iterations;
}

template<class T, class U>
double cfr_solver<T, U>::update(const game_state& state, const bucket_t& buckets, std::array<double, 2>& reach, const int result)
{
    const int player = state.get_player();
    const int opponent = player ^ 1;
    const int bucket = buckets[player][state.get_round()];
    std::array<double, ACTIONS> action_probabilities;

    assert(bucket >= 0 && bucket < abstraction_.get_bucket_count(state.get_round()));

    get_regret_strategy(state, bucket, action_probabilities);

    if (reach[player] > EPSILON)
    {
        auto& strategy = strategy_[state.get_id()][bucket];

        for (int i = 0; i < ACTIONS; ++i)
        {
            assert(state.get_child(i) || action_probabilities[i] == 0);

            // update average strategy
            if (action_probabilities[i] > EPSILON)
                strategy[i] += reach[player] * action_probabilities[i];
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
        auto& regrets = regrets_[state.get_id()][bucket];

        for (int i = 0; i < ACTIONS; ++i)
        {
            if (!state.get_child(i))
                continue;

            // counterfactual regret
            double delta_regret = (action_ev[i] - total_ev) * reach[opponent];

            if (player == 1)
                delta_regret = -delta_regret; // invert sign for P2

            regrets[i] += delta_regret;

            if (delta_regret > EPSILON)
                accumulated_regret_[player] += delta_regret;
        }
    }

    return total_ev;
}

template<class T, class U>
void cfr_solver<T, U>::get_regret_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out)
{
    const auto& bucket_regret = regrets_[state.get_id()][bucket];
    double bucket_sum = 0;

    for (int i = 0; i < ACTIONS; ++i)
    {
        assert(state.get_child(i) || bucket_regret[i] <= EPSILON);

        if (bucket_regret[i] > EPSILON)
            bucket_sum += bucket_regret[i];
    }

    if (bucket_sum > EPSILON)
    {
        for (int i = 0; i < ACTIONS; ++i)
            out[i] = bucket_regret[i] > EPSILON ? bucket_regret[i] / bucket_sum : 0;
    }
    else
    {
        for (int i = 0; i < ACTIONS; ++i)
            out[i] = state.get_child(i) ? 1.0 / state.get_child_count() : 0.0;
    }
}

template<class T, class U>
void cfr_solver<T, U>::get_average_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out) const
{
    const auto& bucket_strategy = strategy_[state.get_id()][bucket];
    double bucket_sum = 0;

    for (int i = 0; i < ACTIONS; ++i)
    {
        assert(state.get_child(i) || bucket_strategy[i] <= EPSILON);
        assert(bucket_strategy[i] >= 0);
        bucket_sum += bucket_strategy[i];
    }

    if (bucket_sum > EPSILON)
    {
        for (int i = 0; i < ACTIONS; ++i)
            out[i] = bucket_strategy[i] / bucket_sum;
    }
    else
    {
        for (int i = 0; i < ACTIONS; ++i)
            out[i] = state.get_child(i) ? 1.0 / state.get_child_count() : 0.0;
    }
}

template<class T, class U>
double cfr_solver<T, U>::get_accumulated_regret(const int player) const
{
    return accumulated_regret_[player];
}

template<class T, class U>
void cfr_solver<T, U>::save_state(std::ostream& os) const
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
            for (int k = 0; k < ACTIONS; ++k)
            {
                os.write(reinterpret_cast<char*>(&regrets_[i][j][k]), sizeof(double));
                os.write(reinterpret_cast<char*>(&strategy_[i][j][k]), sizeof(double));
            }
        }
    }
}

template<class T, class U>
void cfr_solver<T, U>::load_state(std::istream& is)
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
            for (int k = 0; k < ACTIONS; ++k)
            {
                is.read(reinterpret_cast<char*>(&regrets_[i][j][k]), sizeof(double));
                is.read(reinterpret_cast<char*>(&strategy_[i][j][k]), sizeof(double));
            }
        }
    }
}

template<class T, class U>
std::ostream& cfr_solver<T, U>::print(std::ostream& os) const
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

            std::array<double, ACTIONS> p;
            get_average_strategy(*states_[i], j, p);

            for (std::size_t k = 0; k < p.size(); ++k)
                os << "," << std::fixed << p[k];

            os << "\n";
        }
    }

    return os;
}

#include "holdem_game.h"
#include "kuhn_game.h"
#include "holdem_state.h"
#include "kuhn_state.h"
#include "nl_holdem_state.h"

template class cfr_solver<holdem_game, holdem_state>;
template class cfr_solver<kuhn_game, kuhn_state>;
template class cfr_solver<holdem_game, nl_holdem_state>;
