#pragma once

#include <array>

class kuhn_abstraction
{
public:
    kuhn_abstraction(const std::array<int, 1>&);
    int get_bucket(int card) const;
    int get_bucket_count(int round) const;
};
