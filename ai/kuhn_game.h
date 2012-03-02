#pragma once

#include "kuhn_state.h"
#include "kuhn_evaluator.h"
#include "kuhn_abstraction.h"

class kuhn_game
{
public:
    typedef kuhn_state state_t;
    typedef std::array<int, state_t::ROUNDS> bucket_count_t;
    typedef std::array<std::array<int, state_t::ROUNDS>, 2> bucket_t;
    typedef kuhn_evaluator evaluator_t;
    typedef kuhn_abstraction abstraction_t;

    kuhn_game();
    int play(const evaluator_t& eval, const abstraction_t& abs, bucket_t* buckets);

private:
    std::mt19937 engine_;
    std::array<int, 3> deck_;
};
