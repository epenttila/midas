#pragma once

#pragma warning(push, 1)
#include <ostream>
#include <boost/noncopyable.hpp>
#include <array>
#include <vector>
#include <cstdint>
#include <boost/signals2.hpp>
#pragma warning(pop)

#include "solver_base.h"

class strategy;

template<class T, class U>
class pcs_cfr_solver : public solver_base, private boost::noncopyable
{
public:
    static const int ACTIONS = U::ACTIONS;
    static const int ROUNDS = T::ROUNDS;

    typedef typename T game_type;
    typedef typename U game_state;
    typedef typename T::abstraction_t::bucket_type bucket_t;
    typedef typename T::evaluator_t evaluator_t;
    typedef typename T::abstraction_t abstraction_t;

    pcs_cfr_solver(std::unique_ptr<game_state> state, std::unique_ptr<abstraction_t> abstraction);
    ~pcs_cfr_solver();
    virtual void solve(const std::uint64_t iterations);
    virtual void save_strategy(const std::string& filename) const;
    virtual void init_storage();
    virtual std::vector<int> get_bucket_counts() const;
    virtual std::vector<int> get_state_counts() const;
    virtual std::size_t get_required_values() const;
    virtual std::size_t get_required_memory() const;
    virtual void connect_progressed(const std::function<void (std::uint64_t)>& f);
    virtual void save_state(const std::string& filename) const;
    virtual void load_state(const std::string& filename);
    virtual void print(std::ostream& os) const;

private:
    static const double EPSILON;
    static const int PRIVATE = T::PRIVATE_OUTCOMES;

    struct data_type
    {
        data_type() : regret(0), strategy(0) {}
        double regret;
        double strategy;
    };

    typedef typename T::buckets_type buckets_type;
    typedef typename T::results_type results_type;
    typedef std::array<std::array<double, PRIVATE>, 2> ev_type;
    typedef std::array<std::array<double, PRIVATE>, 2> reach_type;

    ev_type update(const game_type& game, const game_state& state, const buckets_type& buckets,
        const reach_type& reach);
    void get_regret_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out) const;
    void get_average_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out) const;
    double get_accumulated_regret(const int player) const;
    data_type* get_data(std::size_t state_id, int bucket, int action);
    const data_type* get_data(std::size_t state_id, int bucket, int action) const;

    std::vector<const game_state*> states_;
    std::vector<data_type> data_;
    std::vector<std::size_t> positions_;
    std::array<double, 2> accumulated_regret_;
    std::unique_ptr<game_state> root_;
    const evaluator_t evaluator_;
    std::unique_ptr<abstraction_t> abstraction_;
    std::uint64_t total_iterations_;
    boost::signals2::signal<void (std::uint64_t)> progressed_;
};

#include "pcs_cfr_solver.ipp"
