#pragma once

#include <random>
#include <boost/noncopyable.hpp>
#include "evallib/kuhn_evaluator.h"
#include "abslib/kuhn_abstraction.h"

class kuhn_game : private boost::noncopyable
{
public:
    enum kuhn_round
    {
        FIRST,
        ROUNDS
    };

    typedef std::array<int, ROUNDS> bucket_count_t;
    typedef std::array<std::array<int, ROUNDS>, 2> bucket_t;
    typedef kuhn_evaluator evaluator_t;
    typedef kuhn_abstraction abstraction_t;

    kuhn_game(const evaluator_t& evaluator, const abstraction_t& abstraction);
    int play(bucket_t* buckets);

private:
    std::mt19937 engine_;
    std::array<int, 3> deck_;
    const evaluator_t& evaluator_;
    const abstraction_t& abstraction_;
};
