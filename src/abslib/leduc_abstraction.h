#pragma once

#include <array>
#include "gamelib/leduc_state.h"

class leduc_abstraction
{
public:
    typedef std::array<int, 2> bucket_type;

    int get_bucket(int card) const;
    int get_bucket_count(leduc_state::game_round round) const;
    void get_buckets(int card, int board, bucket_type* buckets) const;
};
