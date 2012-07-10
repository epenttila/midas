#pragma once

#include "holdem_evaluator.h"
#include "holdem_flop_lut.h"
#include "holdem_turn_lut.h"
#include "holdem_river_lut.h"
#include "holdem_preflop_lut.h"

class holdem_abstraction
{
public:
    typedef std::array<int, 4> bucket_type;
    typedef holdem_evaluator evaluator;

    holdem_abstraction(const std::string& bucket_configuration);
    void get_buckets(int c0, int c1, int b0, int b1, int b2, int b3, int b4, bucket_type* buckets) const;
    int get_bucket_count(int round) const;

private:
    struct bucket_cfg
    {
        enum { PERFECT_RECALL } type;
        int size;
    };

    int get_preflop_bucket(int c0, int c1) const;
    int get_private_flop_bucket(int c0, int c1, int b0, int b1, int b2) const;
    int get_private_turn_bucket(int c0, int c1, int b0, int b1, int b2, int b3) const;
    int get_private_river_bucket(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const;

    const std::unique_ptr<evaluator> evaluator_;
    holdem_preflop_lut preflop_lut_;
    holdem_flop_lut flop_lut_;
    holdem_turn_lut turn_lut_;
    holdem_river_lut river_lut_;
    std::vector<float> preflop_ehs2_percentiles_;
    std::vector<float> flop_ehs2_percentiles_;
    std::vector<float> turn_ehs2_percentiles_;
    std::vector<float> river_ehs_percentiles_;
    std::vector<int> public_flop_buckets_;
    std::array<bucket_cfg, 4> bucket_cfgs_;
};
