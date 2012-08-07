//#include <omp.h>
#include "util/binary_io.h"

template<class T, class U>
pcs_cfr_solver<T, U>::pcs_cfr_solver(std::unique_ptr<game_state> state, std::unique_ptr<abstraction_t> abstraction)
    : root_(std::move(state))
    , evaluator_()
    , abstraction_(std::move(abstraction))
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

    assert(states_.size() > 1); // invalid game tree
    assert(root_->get_child_count() > 0);
}

template<class T, class U>
pcs_cfr_solver<T, U>::~pcs_cfr_solver()
{
}

template<class T, class U>
void pcs_cfr_solver<T, U>::solve(const std::uint64_t iterations)
{
    const double start_time = omp_get_wtime();
    double time = start_time;
    std::uint64_t iteration = 0;

    progressed_(iteration);

#pragma omp parallel
    {
        T g;

        buckets_type buckets;
        results_type results;

        // TODO make unsigned when OpenMP 3.0 is supported
#pragma omp for schedule(dynamic)
        for (std::int64_t i = 0; i < std::int64_t(iterations); ++i)
        {
            g.play_public(evaluator_, *abstraction_, buckets, results);

            ev_type ev = {{}};
            reach_type reach;
            std::for_each(reach.begin(), reach.end(), [](reach_type::value_type& v) { v.fill(1.0); });

            update(*states_[0], buckets, reach, results, ev);

#pragma omp atomic
            ++iteration;

            const double t = omp_get_wtime();

            if (omp_get_thread_num() == 0 && t - time >= 1)
            {
                progressed_(iteration);
                time = t;
            }
        }
    }

    progressed_(iteration);

    total_iterations_ += iterations;
}

template<class T, class U>
void pcs_cfr_solver<T, U>::update(const game_state& state, const buckets_type& buckets, const reach_type& reaches,
    const results_type& results, ev_type& total_ev)
{
    const int player = state.get_player();
    const int opponent = player ^ 1;
    const buckets_type::value_type& round_buckets = buckets[state.get_round()];

    std::array<std::array<double, ACTIONS>, PRIVATE> action_probabilities;
    std::memset(&action_probabilities[0], 0, ACTIONS * PRIVATE * sizeof(double));

    for (int p = 0; p < PRIVATE; ++p)
    {
        const int bucket = round_buckets[p];

        if (bucket == -1)
            continue;

        assert(bucket >= 0 && bucket < abstraction_->get_bucket_count(state.get_round()));

        get_regret_strategy(state, bucket, action_probabilities[p]);

        const reach_type::value_type& reach = reaches[p];

        if (reach[player] > EPSILON)
        {
            auto data = get_data(state.get_id(), bucket, 0);

            for (int i = 0; i < ACTIONS; ++i)
            {
                assert(state.get_child(i) || action_probabilities[p][i] == 0);

                // update average strategy
                if (action_probabilities[p][i] > EPSILON)
                    data[i].strategy += reach[player] * action_probabilities[p][i];
            }
        }
    }

    std::array<ev_type, ACTIONS> action_ev;
    std::memset(&action_ev[0], 0, ACTIONS * PRIVATE * sizeof(double));

    for (int i = 0; i < ACTIONS; ++i)
    {
        // handle next state
        const game_state* next = state.get_child(i);

        if (!next)
            continue; // impossible action

        if (next->is_terminal())
        {
            for (int hole = 0; hole < PRIVATE; ++hole)
            {
                const results_type::value_type& result = results[hole];

                if (result.win == -1)
                    continue;

                action_ev[i][hole] =
                    (result.win * next->get_terminal_ev(1)
                    + result.tie * next->get_terminal_ev(0)
                    + result.lose * next->get_terminal_ev(-1))
                    / double(result.win + result.tie + result.lose);
            }
        }
        else
        {
            reach_type new_reaches;

            for (int p = 0; p < PRIVATE; ++p)
            {
                new_reaches[p][player] = reaches[p][player] * action_probabilities[p][i];
                new_reaches[p][opponent] = reaches[p][opponent];
            }

            update(*next, buckets, new_reaches, results, action_ev[i]);
        }

        for (int p = 0; p < PRIVATE; ++p)
            total_ev[p] += action_probabilities[p][i] * action_ev[i][p];
    }

    // update regrets
    for (int p = 0; p < PRIVATE; ++p)
    {
        const int bucket = round_buckets[p];

        if (bucket == -1)
            continue;

        const reach_type::value_type& reach = reaches[p];

        if (reach[opponent] > EPSILON)
        {
            auto data = get_data(state.get_id(), bucket, 0);

            for (int i = 0; i < ACTIONS; ++i)
            {
                if (!state.get_child(i))
                    continue;

                // counterfactual regret
                double delta_regret = (action_ev[i][p] - total_ev[p]) * reach[opponent];

                if (player == 1)
                    delta_regret = -delta_regret; // invert sign for P2

                data[i].regret += delta_regret;

                if (delta_regret > EPSILON)
                    accumulated_regret_[player] += delta_regret;
            }
        }
    }
}

template<class T, class U>
void pcs_cfr_solver<T, U>::get_regret_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out) const
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

template<class T, class U>
void pcs_cfr_solver<T, U>::get_average_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out) const
{
    const auto data = get_data(state.get_id(), bucket, 0);
    double bucket_sum = 0;

    for (int i = 0; i < ACTIONS; ++i)
    {
        assert(state.get_child(i) || data[i].strategy <= EPSILON);
        assert(data[i].strategy >= 0);
        bucket_sum += data[i].strategy;
    }

    if (bucket_sum > EPSILON)
    {
        for (int i = 0; i < ACTIONS; ++i)
            out[i] = data[i].strategy / bucket_sum;
    }
    else
    {
        for (int i = 0; i < ACTIONS; ++i)
            out[i] = state.get_child(i) ? 1.0 / state.get_child_count() : 0.0;
    }
}

template<class T, class U>
double pcs_cfr_solver<T, U>::get_accumulated_regret(const int player) const
{
    return accumulated_regret_[player];
}

template<class T, class U>
void pcs_cfr_solver<T, U>::save_state(const std::string& filename) const
{
    std::ofstream os(filename, std::ios::binary);
    binary_write(os, total_iterations_);
    binary_write(os, accumulated_regret_);
    binary_write(os, data_);
}

template<class T, class U>
void pcs_cfr_solver<T, U>::load_state(const std::string& filename)
{
    std::ifstream is(filename, std::ios::binary);

    if (!is)
        return;

    binary_read(is, total_iterations_);
    binary_read(is, accumulated_regret_);
    binary_read(is, data_);
}

template<class T, class U>
void pcs_cfr_solver<T, U>::save_strategy(const std::string& filename) const
{
    std::unique_ptr<FILE, int (*)(FILE*)> file(fopen(filename.c_str(), "wb"), fclose);
    std::vector<std::size_t> pointers;

    for (auto i = states_.begin(); i != states_.end(); ++i)
    {
        const auto& state = *i;
        assert(!state->is_terminal() && std::distance(states_.begin(), i) == state->get_id());
        pointers.push_back(_ftelli64(file.get()));

        for (int bucket = 0; bucket < abstraction_->get_bucket_count(state->get_round()); ++bucket)
        {
            std::array<double, ACTIONS> p;
            get_average_strategy(*state, bucket, p);
            fwrite(reinterpret_cast<char*>(&p[0]), sizeof(p[0]), p.size(), file.get());
        }
    }

    fwrite(reinterpret_cast<char*>(&pointers[0]), sizeof(pointers[0]), pointers.size(), file.get());
}

template<class T, class U>
void pcs_cfr_solver<T, U>::init_storage()
{
    data_.resize(get_required_values());
    positions_.resize(states_.size());
    std::size_t pos = 0;

    for (auto i = states_.begin(); i != states_.end(); ++i)
    {
        positions_[(*i)->get_id()] = pos;
        pos += abstraction_->get_bucket_count((*i)->get_round()) * ACTIONS;
    }

    assert(pos == data_.size());
}

template<class T, class U>
std::vector<int> pcs_cfr_solver<T, U>::get_bucket_counts() const
{
    std::vector<int> counts;

    for (int i = 0; i < ROUNDS; ++i)
        counts.push_back(abstraction_->get_bucket_count(i));

    return counts;
}

template<class T, class U>
std::vector<int> pcs_cfr_solver<T, U>::get_state_counts() const
{
    std::vector<int> counts(ROUNDS);
    std::for_each(states_.begin(), states_.end(), [&](const game_state* s) { ++counts[s->get_round()]; });
    return counts;
}

template<class T, class U>
std::size_t pcs_cfr_solver<T, U>::get_required_values() const
{
    std::size_t n = 0;

    for (auto i = states_.begin(); i != states_.end(); ++i)
        n += abstraction_->get_bucket_count((*i)->get_round()) * ACTIONS;

    return n;
}

template<class T, class U>
std::size_t pcs_cfr_solver<T, U>::get_required_memory() const
{
    return get_required_values() * sizeof(data_type);
}

template<class T, class U>
void pcs_cfr_solver<T, U>::connect_progressed(const std::function<void (std::uint64_t)>& f)
{
    progressed_.connect(f);
}

template<class T, class U>
typename pcs_cfr_solver<T, U>::data_type* pcs_cfr_solver<T, U>::get_data(std::size_t state_id, int bucket, int action)
{
    return const_cast<data_type*>(const_cast<const pcs_cfr_solver<T, U>&>(*this).get_data(state_id, bucket, action));
}

template<class T, class U>
const typename pcs_cfr_solver<T, U>::data_type* pcs_cfr_solver<T, U>::get_data(std::size_t state_id, int bucket, int action) const
{
    assert(state_id >= 0 && state_id < states_.size());
    assert(bucket >= 0 && bucket < abstraction_->get_bucket_count(states_[state_id]->get_round()));

    return &data_[positions_[state_id] + bucket * ACTIONS + action];
}

template<class T, class U>
const double pcs_cfr_solver<T, U>::EPSILON = 1e-7;
