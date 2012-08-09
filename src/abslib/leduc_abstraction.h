#pragma once

#include <array>

class leduc_abstraction
{
public:
    typedef std::array<int, 2> bucket_type;

    int get_bucket(int card) const;
    int get_bucket_count(int round) const;
    void get_buckets(int card, int board, bucket_type* buckets) const;
};
