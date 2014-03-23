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

#include "cfr_solver.h"

class strategy;

template<class T, class U>
class pure_cfr_solver : public cfr_solver<T, U, std::int32_t>
{
public:
    typedef cfr_solver<T, U, std::int32_t> base_t;
    typedef typename base_t::game_state game_state;
    typedef typename base_t::abstraction_t abstraction_t;
    typedef typename base_t::data_t data_t;
    typedef typename base_t::bucket_t bucket_t;
    typedef typename base_t::cfr_t cfr_t;

    static const int ACTIONS = game_state::ACTIONS;

    pure_cfr_solver(std::unique_ptr<game_state> state, std::unique_ptr<abstraction_t> abstraction);
    ~pure_cfr_solver();

private:
    data_t update(int position, std::mt19937& engine, const game_state& state, const bucket_t& buckets,
        const int result, cfr_t* cfr);
    int get_regret_strategy(std::mt19937& engine, const game_state& state, const int bucket) const;
    virtual void run_iteration(T& game, cfr_t* cfr);
};

#include "pure_cfr_solver.ipp"
