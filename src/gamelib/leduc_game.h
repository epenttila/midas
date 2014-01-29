#pragma once

#include <random>
#include <array>
#include <cstdint>
#include <boost/noncopyable.hpp>
#include "abslib/leduc_abstraction.h"
#include "evallib/leduc_evaluator.h"

class leduc_game : private boost::noncopyable
{
public:
    static const int CARDS = 6;

    typedef std::array<std::array<int, leduc_state::ROUNDS>, 2> bucket_t;
    typedef leduc_evaluator evaluator_t;
    typedef leduc_abstraction abstraction_t;

    leduc_game(const evaluator_t& evaluator, const abstraction_t& abstraction, std::int64_t seed);
    int play(bucket_t* buckets);
    std::mt19937& get_random_engine();

private:
    std::mt19937 engine_;
    std::array<int, CARDS> deck_;
    const evaluator_t& evaluator_;
    const abstraction_t& abstraction_;
};
