#pragma once

#include "cfr_solver.h"

template<class T, class U>
class pcs_cfr_solver : public cfr_solver<T, U, double>
{
public:
    typedef cfr_solver<T, U, double> base_t;
    typedef typename base_t::game_type game_type;
    typedef typename base_t::game_state game_state;
    typedef typename base_t::abstraction_t abstraction_t;

    static const int ACTIONS = base_t::ACTIONS;

    pcs_cfr_solver(std::unique_ptr<game_state> state, std::unique_ptr< abstraction_t> abstraction);
    ~pcs_cfr_solver();

private:
    static const int PRIVATE = T::PRIVATE_OUTCOMES;

    typedef typename T::buckets_type buckets_type;
    typedef std::array<std::array<double, PRIVATE>, 2> ev_type;
    typedef std::array<std::array<double, PRIVATE>, 2> reach_type;

    void update(const game_type& game, const game_state& state, const buckets_type& buckets,
        const reach_type& reach,  ev_type& utility);
    void get_regret_strategy(const game_state& state, const int bucket, std::array<double, ACTIONS>& out) const;
    virtual void run_iteration(T& game);
};

#include "pcs_cfr_solver.ipp"
