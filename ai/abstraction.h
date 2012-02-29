#pragma once

#include "holdem_state.h"

class evaluator;

class abstraction
{
public:
    typedef std::array<int, holdem_state::ROUNDS> bucket_count_t;

    abstraction(const bucket_count_t& bucket_counts);
    int get_preflop_bucket(int c0, int c1) const;
    int get_flop_bucket(int c0, int c1, int b0, int b1, int b2) const;
    int get_turn_bucket(int c0, int c1, int b0, int b1, int b2, int b3) const;
    int get_river_bucket(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const;

private:
    std::array<std::array<int, 13>, 13> preflop_;
    std::array<int, holdem_state::ROUNDS> bucket_counts_;
    const std::unique_ptr<evaluator> evaluator_;
    static const int ITERATIONS;
};
