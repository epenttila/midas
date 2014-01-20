#include "twoplustwo_evaluator.h"
#include <fstream>
#include <stdexcept>

const int twoplustwo_evaluator::RANKS = 32487834;
std::unique_ptr<int[]> twoplustwo_evaluator::ranks_ = std::unique_ptr<int[]>(new int[RANKS]);

twoplustwo_evaluator::twoplustwo_evaluator()
{
    std::ifstream f("ranks.dat", std::ios_base::binary);
    
    if (!f)
        throw std::runtime_error("ranks.dat not found");

    f.read(reinterpret_cast<char*>(&ranks_[0]), sizeof(ranks_[0]) * RANKS);
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
