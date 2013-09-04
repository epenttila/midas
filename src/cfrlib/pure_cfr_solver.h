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

template<class T, class U>
class pure_cfr_solver : public cfr_solver<T, U, std::int32_t>
{
public:
    pure_cfr_solver(std::unique_ptr<game_state> state, std::unique_ptr<abstraction_t> abstraction);
    ~pure_cfr_solver();

private:
    data_t update(int position, std::mt19937& engine, const game_state& state, const bucket_t& buckets, const int result);
    int get_regret_strategy(std::mt19937& engine, const game_state& state, const int bucket) const;
    virtual void run_iteration(T& game);
};

#include "pure_cfr_solver.ipp"
