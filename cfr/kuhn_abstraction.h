#pragma once

#include <array>

class kuhn_abstraction
{
public:
    kuhn_abstraction(const std::string&);
    int get_bucket(int card) const;
    int get_bucket_count(int round) const;
};
