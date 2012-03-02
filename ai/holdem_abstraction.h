#pragma once

#include "holdem_state.h"

class holdem_evaluator;

class holdem_abstraction
{
public:
    holdem_abstraction(const std::array<int, 4>&);
    int get_preflop_bucket(int c0, int c1) const;
    int get_flop_bucket(int c0, int c1, int b0, int b1, int b2) const;
    int get_turn_bucket(int c0, int c1, int b0, int b1, int b2, int b3) const;
    int get_river_bucket(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const;
    int get_bucket_count(int round) const;

private:
    std::array<std::array<int, 13>, 13> preflop_;
    const std::unique_ptr<holdem_evaluator> evaluator_;
    static const int ITERATIONS;
};
