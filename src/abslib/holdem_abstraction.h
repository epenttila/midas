#pragma once

#include "evallib/holdem_evaluator.h"
#include "lutlib/holdem_flop_lut.h"
#include "lutlib/holdem_turn_lut.h"
#include "lutlib/holdem_river_lut.h"
#include "lutlib/holdem_preflop_lut.h"

class holdem_abstraction
{
public:
    typedef std::array<int, 4> bucket_type;
    typedef holdem_evaluator evaluator;
    enum holdem_round { PREFLOP, FLOP, TURN, RIVER, ROUNDS };

    holdem_abstraction(const std::string& bucket_configuration, int kmeans_max_iterations);
    holdem_abstraction(std::istream&& is);
    void get_buckets(int c0, int c1, int b0, int b1, int b2, int b3, int b4, bucket_type* buckets) const;
    int get_bucket(int c0, int c1) const;
    int get_bucket(int c0, int c1, int b0, int b1, int b2) const;
    int get_bucket(int c0, int c1, int b0, int b1, int b2, int b3) const;
    int get_bucket(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const;
    int get_bucket_count(int round) const;

    void save(std::ostream& os) const;
    void load(std::istream& is);

private:
    struct bucket_cfg
    {
        bucket_cfg();
        int hs2;
        int pub;
        bool forget_hs2;
        bool forget_pub;
    };

    void init();

    int get_preflop_bucket(int c0, int c1) const;
    int get_private_flop_bucket(int c0, int c1, int b0, int b1, int b2) const;
    int get_private_turn_bucket(int c0, int c1, int b0, int b1, int b2, int b3) const;
    int get_private_river_bucket(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const;

    int get_public_flop_bucket(int b0, int b1, int b2) const;

    void generate_public_flop_buckets(int kmeans_max_iterations, int kmeans_buckets);
    void generate_public_turn_buckets(int kmeans_max_iterations, int kmeans_buckets);

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
    std::array<bucket_cfg, 4> bucket_cfgs_;
};
