#pragma once

#pragma warning(push, 1)
#include <ostream>
#include <boost/noncopyable.hpp>
#include <array>
#include <vector>
#include <cstdint>
#include <boost/signals2.hpp>
#pragma warning(pop)

class strategy;

class solver_base
{
    friend std::ostream& operator<<(std::ostream& o, const solver_base& solver);

public:
    virtual void solve(const int iterations) = 0;
    virtual void save_state(std::ostream&) const = 0;
    virtual void load_state(std::istream&) = 0;
    virtual void save_strategy(const std::string& filename) const = 0;
    virtual void init_storage() = 0;
    virtual std::vector<int> get_bucket_counts() const = 0;
    virtual std::vector<int> get_state_counts() const = 0;
    virtual std::size_t get_required_memory() const = 0;
    virtual void connect_progressed(const std::function<void (std::uint64_t)>& f) = 0;
};

template<class T, class U>
class cfr_solver : public solver_base, private boost::noncopyable
{
public:
    static const int ACTIONS = U::ACTIONS;
    static const int ROUNDS = T::ROUNDS;

    typedef typename U game_state;
    typedef typename T::bucket_t bucket_t;
    typedef typename T::evaluator_t evaluator_t;
    typedef typename T::abstraction_t abstraction_t;

    cfr_solver(abstraction_t abstraction, int stack_size);
    ~cfr_solver();
    virtual void solve(const int iterations);
    virtual void save_strategy(const std::string& filename) const;
    virtual void init_storage();
    virtual std::vector<int> get_bucket_counts() const;
    virtual std::vector<int> get_state_counts() const;
    virtual std::size_t get_required_memory() const;
    virtual void connect_progressed(const std::function<void (std::uint64_t)>& f);

private:
    double update(const game_state& state, const bucket_t& buckets, std::array<double, 2>& reach, const int result);
    void get_regret_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out);
    void get_average_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out) const;
    double get_accumulated_regret(const int player) const;
    void save_state(std::ostream& os) const;
    void load_state(std::istream& is);

    std::vector<const game_state*> states_;
    typedef double (*value)[ACTIONS];
    value* regrets_;
    value* strategy_;
    std::array<double, 2> accumulated_regret_;
    std::unique_ptr<game_state> root_;
    const evaluator_t evaluator_;
    const abstraction_t abstraction_;
    int total_iterations_;
    boost::signals2::signal<void (std::uint64_t)> progressed_;
};
