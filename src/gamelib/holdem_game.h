#pragma once

#include <random>
#include <cstdint>
#include "evallib/holdem_evaluator.h"
#include "holdem_state.h"

template<class Abstraction>
class holdem_game : private boost::noncopyable
{
public:
    static const int ALLIN_BET_SIZE = 999;

    typedef std::array<std::array<int, holdem_state::ROUNDS>, 2> bucket_t;
    typedef holdem_evaluator evaluator_t;
    typedef Abstraction abstraction_t;

    holdem_game(const evaluator_t& eval, const abstraction_t& abs, std::int64_t seed);
    int play(bucket_t* buckets);
    std::mt19937& get_random_engine();

private:
    std::mt19937 engine_;
    std::array<int, 52> deck_;
    const evaluator_t& evaluator_;
    const abstraction_t& abstraction_;
};

#include "holdem_game.ipp"
