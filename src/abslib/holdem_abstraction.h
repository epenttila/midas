#pragma once

#include "evallib/holdem_evaluator.h"
#include "lutlib/holdem_flop_lut.h"
#include "lutlib/holdem_turn_lut.h"
#include "lutlib/holdem_river_lut.h"
#include "lutlib/holdem_preflop_lut.h"
#include "util/game.h"
#include "holdem_abstraction_base.h"

class holdem_abstraction : public holdem_abstraction_base
{
public:
    typedef std::array<int, 4> bucket_type;
    typedef holdem_evaluator evaluator;

    static const int ROUNDS = RIVER + 1;

    struct bucket_cfg
    {
        bucket_cfg();
        int hs2;
        int pub;
        bool forget_hs2;
        bool forget_pub;
    };

    typedef std::array<bucket_cfg, ROUNDS> bucket_cfg_type;

    holdem_abstraction();
    void get_buckets(int c0, int c1, int b0, int b1, int b2, int b3, int b4, bucket_type* buckets) const;
    int get_bucket_count(int round) const;

    void write(const std::string& filename) const;
    void read(const std::string& filename);
    void generate(const std::string& configuration, const int kmeans_max_iterations, float tolerance, int runs);

private:
    int get_preflop_bucket(int c0, int c1) const;
    int get_private_flop_bucket(int c0, int c1, int b0, int b1, int b2) const;
    int get_private_turn_bucket(int c0, int c1, int b0, int b1, int b2, int b3) const;
    int get_private_river_bucket(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const;

    int get_public_flop_bucket(int b0, int b1, int b2) const;
    int get_public_turn_bucket(int b0, int b1, int b2, int b3) const;

    std::shared_ptr<evaluator> evaluator_;
    std::shared_ptr<holdem_preflop_lut> preflop_lut_;
    std::shared_ptr<holdem_flop_lut> flop_lut_;
    std::shared_ptr<holdem_turn_lut> turn_lut_;
    std::shared_ptr<holdem_river_lut> river_lut_;
    std::vector<float> preflop_ehs2_percentiles_;
    std::vector<float> flop_ehs2_percentiles_;
    std::vector<float> turn_ehs2_percentiles_;
    std::vector<float> river_ehs_percentiles_;
    std::vector<int> public_flop_buckets_;
    std::vector<int> public_turn_buckets_;
    bucket_cfg_type bucket_cfgs_;
};
