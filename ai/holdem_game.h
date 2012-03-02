#pragma once

#include "holdem_state.h"
#include "holdem_evaluator.h"
#include "holdem_abstraction.h"

class holdem_game
{
public:
    typedef holdem_state state_t;
    typedef std::array<int, state_t::ROUNDS> bucket_count_t;
    typedef std::array<std::array<int, state_t::ROUNDS>, 2> bucket_t;
    typedef holdem_evaluator evaluator_t;
    typedef holdem_abstraction abstraction_t;

    holdem_game();
    int play(const evaluator_t& eval, const abstraction_t& abs, bucket_t* buckets);

private:
    std::mt19937 engine_;
    std::array<int, 52> deck_;
};
