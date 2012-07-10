#pragma once

#include <random>
#include "holdem_evaluator.h"
#include "holdem_abstraction.h"

class holdem_game
{
public:
    enum holdem_round
    {
        PREFLOP,
        FLOP,
        TURN,
        RIVER,
        ROUNDS
    };

    typedef std::array<std::array<int, ROUNDS>, 2> bucket_t;
    typedef holdem_evaluator evaluator_t;
    typedef holdem_abstraction abstraction_t;

    holdem_game();
    int play(const evaluator_t& eval, const abstraction_t& abs, bucket_t* buckets);

private:
    std::mt19937 engine_;
    std::array<int, 52> deck_;
};
