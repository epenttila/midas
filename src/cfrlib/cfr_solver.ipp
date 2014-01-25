#include <omp.h>
#include "util/binary_io.h"

#ifdef _MSC_VER
#define ftello _ftelli64
#endif

template<class T, class U, class Data>
cfr_solver<T, U, Data>::cfr_solver(std::unique_ptr<game_state> state, std::unique_ptr<abstraction_t> abstraction)
    : root_(std::move(state))
    , evaluator_()
    , abstraction_(std::move(abstraction))
    , total_iterations_(0)
{
    for (const auto p : game_state::get_state_vector(*root_))
        states_.push_back(dynamic_cast<const game_state*>(p));

    assert(states_.size() > 0); // invalid game tree
    assert(root_->get_child_count() > 0);
    assert(states_[0] == root_.get());
}

template<class T, class U, class Data>
cfr_solver<T, U, Data>::~cfr_solver()
{
}

template<class T, class U, class Data>
void cfr_solver<T, U, Data>::solve(const std::uint64_t iterations, std::int64_t seed, int threads)
{
    omp_set_num_threads(threads != -1 ? threads : omp_get_max_threads());

    const double start_time = omp_get_wtime();
    double time = start_time;
    std::uint64_t iteration = 0;

    progressed_(iteration);

#pragma omp parallel
    {
        T g(evaluator_, *abstraction_, seed + iterations);

#pragma omp for
        // TODO make unsigned when OpenMP 3.0 is supported
        for (std::int64_t i = 0; i < std::int64_t(iterations); ++i)
        {
            run_iteration(g);

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

template<class T, class U, class Data>
void cfr_solver<T, U, Data>::get_average_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out) const
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

template<class T, class U, class Data>
void cfr_solver<T, U, Data>::save_state(const std::string& filename) const
{
    auto file = binary_open(filename, "wb");

    if (!file)
        throw std::runtime_error("unable to create state file");

    binary_write(*file, total_iterations_);
    binary_write(*file, data_);

    BOOST_LOG_TRIVIAL(info) << "State with " << total_iterations_ << " iterations saved";
}

template<class T, class U, class Data>
void cfr_solver<T, U, Data>::load_state(const std::string& filename)
{
    auto file = binary_open(filename, "rb");

    if (!file)
        return;

    binary_read(*file, total_iterations_);
    binary_read(*file, data_);

    BOOST_LOG_TRIVIAL(info) << "State with " << total_iterations_ << " iterations loaded";
}

template<class T, class U, class Data>
void cfr_solver<T, U, Data>::save_strategy(const std::string& filename) const
{
    std::unique_ptr<FILE, int (*)(FILE*)> file(fopen(filename.c_str(), "wb"), fclose);
    std::vector<std::size_t> pointers;

    for (auto i = states_.begin(); i != states_.end(); ++i)
    {
        const auto& state = *i;
        assert(!state->is_terminal() && std::distance(states_.begin(), i) == state->get_id());
        pointers.push_back(ftello(file.get()));

        for (int bucket = 0; bucket < abstraction_->get_bucket_count(state->get_round()); ++bucket)
        {
            std::array<double, ACTIONS> p;
            get_average_strategy(*state, bucket, p);
            fwrite(reinterpret_cast<char*>(&p[0]), sizeof(p[0]), p.size(), file.get());
        }
    }

    fwrite(reinterpret_cast<char*>(&pointers[0]), sizeof(pointers[0]), pointers.size(), file.get());
}

template<class T, class U, class Data>
void cfr_solver<T, U, Data>::init_storage()
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

template<class T, class U, class Data>
std::vector<int> cfr_solver<T, U, Data>::get_bucket_counts() const
{
    std::vector<int> counts;

    for (int i = 0; i < ROUNDS; ++i)
        counts.push_back(static_cast<int>(abstraction_->get_bucket_count(i)));

    return counts;
}

template<class T, class U, class Data>
std::vector<int> cfr_solver<T, U, Data>::get_state_counts() const
{
    std::vector<int> counts(ROUNDS);
    std::for_each(states_.begin(), states_.end(), [&](const game_state* s) { ++counts[s->get_round()]; });
    return counts;
}

template<class T, class U, class Data>
std::size_t cfr_solver<T, U, Data>::get_required_values() const
{
    std::size_t n = 0;

    for (auto i = states_.begin(); i != states_.end(); ++i)
        n += abstraction_->get_bucket_count((*i)->get_round()) * ACTIONS;

    return n;
}

template<class T, class U, class Data>
std::size_t cfr_solver<T, U, Data>::get_required_memory() const
{
    return get_required_values() * sizeof(data_type);
}

template<class T, class U, class Data>
void cfr_solver<T, U, Data>::connect_progressed(const std::function<void (std::uint64_t)>& f)
{
    progressed_.connect(f);
}

template<class T, class U, class Data>
typename cfr_solver<T, U, Data>::data_type* cfr_solver<T, U, Data>::get_data(std::size_t state_id, int bucket, int action)
{
    return const_cast<data_type*>(const_cast<const cfr_solver<T, U, Data>&>(*this).get_data(state_id, bucket, action));
}

template<class T, class U, class Data>
const typename cfr_solver<T, U, Data>::data_type* cfr_solver<T, U, Data>::get_data(std::size_t state_id, int bucket, int action) const
{
    assert(state_id < states_.size());
    assert(bucket >= 0 && bucket < abstraction_->get_bucket_count(states_[state_id]->get_round()));

    return &data_[positions_[state_id] + bucket * ACTIONS + action];
}

template<class T, class U, class Data>
void cfr_solver<T, U, Data>::print(std::ostream& os) const
{
    for (auto it = states_.begin(); it != states_.end(); ++it)
    {
        const auto& state = **it;

        for (int bucket = 0; bucket < abstraction_->get_bucket_count(state.get_round()); ++bucket)
        {
            os << state << ":" << bucket << ": ";

            std::array<double, ACTIONS> p;
            get_average_strategy(state, bucket, p);

            for (std::size_t action = 0; action < p.size(); ++action)
                os << (action > 0 ? ", " : "") << p[action];

            os << "\n";
        }
    }
}

template<class T, class U, class Data>
const typename cfr_solver<T, U, Data>::game_state& cfr_solver<T, U, Data>::get_root_state() const
{
    return *root_;
}

template<class T, class U, class Data>
const typename cfr_solver<T, U, Data>::abstraction_t& cfr_solver<T, U, Data>::get_abstraction() const
{
    return *abstraction_;
}
