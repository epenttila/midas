#pragma once

#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <ostream>
#include <boost/noncopyable.hpp>
#include <array>
#include <vector>
#include <cstdint>
#include <boost/signals2.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "solver_base.h"
#include "strategy.h"

class strategy;

template<class T, class U, class Data>
class cfr_solver : public solver_base, private boost::noncopyable
{
public:
    static const int ROUNDS = T::ROUNDS;

    typedef T game_type;
    typedef U game_state;
    typedef typename T::bucket_t bucket_t;
    typedef typename T::evaluator_t evaluator_t;
    typedef typename T::abstraction_t abstraction_t;
    typedef Data data_t;
    typedef strategy::probability_t probability_t;

    cfr_solver(std::unique_ptr<game_state> state, std::unique_ptr<abstraction_t> abstraction);
    ~cfr_solver();
    virtual void solve(const std::uint64_t iterations, std::int64_t seed, int threads = -1);
    virtual void save_strategy(const std::string& filename) const;
    virtual void init_storage();
    virtual std::vector<int> get_bucket_counts() const;
    virtual std::vector<int> get_state_counts() const;
    virtual std::vector<int> get_action_counts() const;
    virtual std::size_t get_required_values() const;
    virtual std::size_t get_required_memory() const;
    virtual void connect_progressed(const std::function<void (std::uint64_t)>& f);
    virtual void save_state(const std::string& filename) const;
    virtual void load_state(const std::string& filename);
    virtual void print(std::ostream& os) const;
    void get_average_strategy(const game_state& state, const int bucket, probability_t* out) const;
    const game_state& get_root_state() const;

protected:
    struct data_type
    {
        data_type() : regret(0), strategy(0) {}
        Data regret;
        Data strategy;
    };

    virtual void run_iteration(T& game) = 0;
    data_type* get_data(std::size_t state_id, int bucket, int action);
    const data_type* get_data(std::size_t state_id, int bucket, int action) const;
    const abstraction_t& get_abstraction() const;

private:
    std::vector<const game_state*> states_;
    std::vector<data_type> data_;
    std::vector<std::size_t> positions_;
    std::unique_ptr<game_state> root_;
    const evaluator_t evaluator_;
    std::unique_ptr<abstraction_t> abstraction_;
    std::uint64_t total_iterations_;
    boost::signals2::signal<void (std::uint64_t)> progressed_;
};

#include "cfr_solver.ipp"
