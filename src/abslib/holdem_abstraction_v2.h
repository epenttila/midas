#pragma once

#include "evallib/holdem_evaluator.h"
#include "lutlib/holdem_flop_lut.h"
#include "lutlib/holdem_turn_lut.h"
#include "lutlib/holdem_river_lut.h"
#include "lutlib/holdem_preflop_lut.h"
#include "lutlib/hand_indexer.h"
#include "lutlib/holdem_river_ochs_lut.h"
#include "util/game.h"

class holdem_abstraction_v2
{
public:
    typedef std::int32_t bucket_idx_t;
    typedef std::array<bucket_idx_t, 4> bucket_type;
    typedef holdem_evaluator evaluator;

    static const int ROUNDS = RIVER + 1;

    typedef std::array<int, ROUNDS> bucket_counts_t;

    holdem_abstraction_v2();
    void get_buckets(int c0, int c1, int b0, int b1, int b2, int b3, int b4, bucket_type* buckets) const;
    int get_bucket(int c0, int c1) const;
    int get_bucket(int c0, int c1, int b0, int b1, int b2) const;
    int get_bucket(int c0, int c1, int b0, int b1, int b2, int b3) const;
    int get_bucket(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const;
    std::size_t get_bucket_count(int round) const;

    void write(const std::string& filename) const;
    void read(const std::string& filename);
    void generate(const std::string& configuration, const int kmeans_max_iterations, float tolerance, int runs);

private:
    void init();
    void parse_configuration(const std::string& configuration);

    int get_preflop_bucket(int c0, int c1) const;
    int get_private_flop_bucket(int c0, int c1, int b0, int b1, int b2) const;
    int get_private_turn_bucket(int c0, int c1, int b0, int b1, int b2, int b3) const;
    int get_private_river_bucket(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const;

    int get_public_flop_bucket(int b0, int b1, int b2) const;
    int get_public_turn_bucket(int b0, int b1, int b2, int b3) const;

    std::unique_ptr<hand_indexer> preflop_indexer_;
    std::unique_ptr<hand_indexer> flop_indexer_;
    std::unique_ptr<hand_indexer> turn_indexer_;
    std::unique_ptr<hand_indexer> river_indexer_;

    bool imperfect_recall_;
    std::vector<bucket_idx_t> preflop_buckets_;
    std::vector<bucket_idx_t> flop_buckets_;
    std::vector<bucket_idx_t> turn_buckets_;
    std::vector<bucket_idx_t> river_buckets_;

    bucket_counts_t bucket_counts_;
};
