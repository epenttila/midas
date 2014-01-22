#pragma once

#include <memory>
#include <array>

class twoplustwo_evaluator
{
public:
    twoplustwo_evaluator();
    int get_hand_value(int c0, int c1, int c2, int c3, int c4) const;
    int get_hand_value(int c0, int c1, int c2, int c3, int c4, int c5) const;
    int get_hand_value(int c0, int c1, int c2, int c3, int c4, int c5, int c6) const;

private:
    static const int RANKS = 32487834;
    std::array<int, RANKS> ranks_;
};
