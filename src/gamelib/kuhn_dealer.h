#pragma once

#include <random>
#include <cstdint>
#include <boost/noncopyable.hpp>
#include "evallib/kuhn_evaluator.h"
#include "abslib/kuhn_abstraction.h"

class kuhn_dealer : private boost::noncopyable
{
public:
    typedef std::array<int, kuhn_state::ROUNDS> bucket_count_t;
    typedef std::array<std::array<int, kuhn_state::ROUNDS>, 2> bucket_t;
    typedef kuhn_evaluator evaluator_t;
    typedef kuhn_abstraction abstraction_t;

    kuhn_dealer(const evaluator_t& evaluator, const abstraction_t& abstraction, std::int64_t seed);
    int play(bucket_t* buckets);
    std::mt19937& get_random_engine();

private:
    std::mt19937 engine_;
    std::array<int, 3> deck_;
    const evaluator_t& evaluator_;
    const abstraction_t& abstraction_;
};
