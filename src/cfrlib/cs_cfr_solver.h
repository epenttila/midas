#pragma once

#include "cfr_solver.h"

template<class T, class U>
class cs_cfr_solver : public cfr_solver<T, U, double>
{
public:
    typedef cfr_solver<T, U, double> base_t;
    typedef typename base_t::game_state game_state;
    typedef typename base_t::abstraction_t abstraction_t;
    typedef typename base_t::bucket_t bucket_t;

    static const int ACTIONS = base_t::ACTIONS;

    cs_cfr_solver(std::unique_ptr<game_state> state, std::unique_ptr<abstraction_t> abstraction);
    ~cs_cfr_solver();

private:
    double update(const game_state& state, const bucket_t& buckets, std::array<double, 2>& reach, const int result);
    void get_regret_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out) const;
    virtual void run_iteration(T& game);
};

#include "cs_cfr_solver.ipp"
