#pragma once

#include <ostream>
#include <boost/noncopyable.hpp>
#include <array>
#include <vector>

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

    cfr_solver(const std::string& bucket_configuration, int stack_size);
    ~cfr_solver();
    virtual void solve(const int iterations);

private:
    double update(const game_state& state, const bucket_t& buckets, std::array<double, 2>& reach, const int result);
    void get_regret_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out);
    void get_average_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out) const;
    double get_accumulated_regret(const int player) const;
    void save_state(std::ostream& os) const;
    void load_state(std::istream& is);
    std::ostream& print(std::ostream& os) const;

    std::vector<const game_state*> states_;
    typedef double (*value)[ACTIONS];
    value* regrets_;
    value* strategy_;
    std::array<double, 2> accumulated_regret_;
    std::unique_ptr<game_state> root_;
    const evaluator_t evaluator_;
    const abstraction_t abstraction_;
    int total_iterations_;
};
