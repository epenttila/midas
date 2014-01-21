#pragma once

class holdem_abstraction_base
{
public:
    typedef int bucket_idx_t;
    typedef std::array<bucket_idx_t, 4> bucket_type;

    virtual void get_buckets(int c0, int c1, int b0, int b1, int b2, int b3, int b4, bucket_type* buckets) const = 0;
    virtual void read(const std::string& filename) = 0;
    virtual int get_bucket_count(int round) const = 0;
};
