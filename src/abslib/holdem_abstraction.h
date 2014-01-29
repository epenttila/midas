#pragma once

#include <boost/iostreams/device/mapped_file.hpp>
#include "evallib/holdem_evaluator.h"
#include "lutlib/hand_indexer.h"
#include "gamelib/holdem_state.h"
#include "holdem_abstraction_base.h"

class holdem_abstraction : public holdem_abstraction_base
{
public:
    typedef holdem_evaluator evaluator;

    typedef std::array<int, holdem_state::ROUNDS> bucket_counts_t;

    holdem_abstraction();
    void get_buckets(int c0, int c1, int b0, int b1, int b2, int b3, int b4, bucket_type* buckets) const;
    int get_bucket_count(holdem_state::game_round round) const;

    void read(const std::string& filename);
    void write(const std::string& filename, const int kmeans_max_iterations, float tolerance, int runs);

private:
    bucket_idx_t read(holdem_state::game_round round, hand_indexer::hand_index_t index) const;

    static const hand_indexer preflop_indexer_;
    static const hand_indexer flop_indexer_;
    static const hand_indexer turn_indexer_;
    static const hand_indexer river_indexer_;

    bool imperfect_recall_;
    bucket_counts_t bucket_counts_;

    boost::iostreams::mapped_file_source file_;
};
