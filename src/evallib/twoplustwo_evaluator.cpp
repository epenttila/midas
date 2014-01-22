#include "twoplustwo_evaluator.h"
#include <stdexcept>

#include "util/binary_io.h"

twoplustwo_evaluator::twoplustwo_evaluator()
{
    auto f = binary_open("ranks.dat", "rb");
    
    if (!f)
        throw std::runtime_error("ranks.dat not found");

    binary_read(*f, ranks_.data(), ranks_.size());
}

int twoplustwo_evaluator::get_hand_value(const int c0, const int c1, const int c2, const int c3, const int c4) const
{
    int p = ranks_[53 + c0 + 1];
    p = ranks_[p + c1 + 1];
    p = ranks_[p + c2 + 1];
    p = ranks_[p + c3 + 1];
    p = ranks_[p + c4 + 1];
    return ranks_[p];
}

int twoplustwo_evaluator::get_hand_value(const int c0, const int c1, const int c2, const int c3, const int c4, const int c5) const
{
    int p = ranks_[53 + c0 + 1];
    p = ranks_[p + c1 + 1];
    p = ranks_[p + c2 + 1];
    p = ranks_[p + c3 + 1];
    p = ranks_[p + c4 + 1];
    p = ranks_[p + c5 + 1];
    return ranks_[p];
}

int twoplustwo_evaluator::get_hand_value(const int c0, const int c1, const int c2, const int c3, const int c4, const int c5,
    const int c6) const
{
    int p = ranks_[53 + c0 + 1];
    p = ranks_[p + c1 + 1];
    p = ranks_[p + c2 + 1];
    p = ranks_[p + c3 + 1];
    p = ranks_[p + c4 + 1];
    p = ranks_[p + c5 + 1];
    return ranks_[p + c6 + 1];
}
