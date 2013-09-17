#pragma once

#include <boost/iostreams/device/mapped_file.hpp>
#include "evallib/holdem_evaluator.h"
#include "lutlib/holdem_flop_lut.h"
#include "lutlib/holdem_turn_lut.h"
#include "lutlib/holdem_river_lut.h"
#include "lutlib/holdem_preflop_lut.h"
#include "lutlib/hand_indexer.h"
#include "lutlib/holdem_river_ochs_lut.h"
#include "util/game.h"
#include "lutlib/hand_indexer.h"
#include "holdem_abstraction_base.h"

class holdem_abstraction_v2 : public holdem_abstraction_base
{
public:
    typedef holdem_evaluator evaluator;

    static const int ROUNDS = RIVER + 1;

    typedef std::array<int, ROUNDS> bucket_counts_t;

    holdem_abstraction_v2();
    void get_buckets(int c0, int c1, int b0, int b1, int b2, int b3, int b4, bucket_type* buckets) const;
    std::size_t get_bucket_count(int round) const;

    void read(const std::string& filename);
    void write(const std::string& filename, const int kmeans_max_iterations, float tolerance, int runs);

private:
    bucket_idx_t read(int round, hand_indexer::hand_index_t index) const;

    static const hand_indexer preflop_indexer_;
    static const hand_indexer flop_indexer_;
    static const hand_indexer turn_indexer_;
    static const hand_indexer river_indexer_;

    bool imperfect_recall_;
    bucket_counts_t bucket_counts_;

    boost::iostreams::mapped_file_source file_;
};
