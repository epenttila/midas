#pragma once

#include "holdem_state.h"
#include "holdem_evaluator.h"

class holdem_abstraction
{
public:
    typedef std::array<int, holdem_state::ROUNDS> bucket_type;
    typedef holdem_evaluator evaluator;

    holdem_abstraction(const bucket_type& bucket_counts);
    void get_buckets(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3, const int b4,
        bucket_type* buckets) const;
    int get_bucket_count(int round) const;

private:
    int get_preflop_bucket(int c0, int c1) const;
    int get_flop_bucket(int c0, int c1, int b0, int b1, int b2) const;
    int get_turn_bucket(int c0, int c1, int b0, int b1, int b2, int b3) const;
    int get_river_bucket(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const;

    const std::unique_ptr<evaluator> evaluator_;
};
