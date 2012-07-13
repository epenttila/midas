#pragma once

#include <boost/noncopyable.hpp>
#include <array>
#include "twoplustwo_evaluator.h"

struct result
{
    double ehs;
    double ehs2;
};

class holdem_evaluator : private boost::noncopyable
{
public:
    holdem_evaluator();
    int get_hand_value(int c0, int c1, int c2, int c3, int c4) const;
    int get_hand_value(int c0, int c1, int c2, int c3, int c4, int c5) const;
    int get_hand_value(int c0, int c1, int c2, int c3, int c4, int c5, int c6) const;
    const result enumerate_preflop(const int c0, const int c1) const;
    const result enumerate_flop(const int c0, const int c1, const int b0, const int b1, const int b2) const;
    const result enumerate_turn(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3) const;
    double enumerate_river(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3, const int b4) const;
    const result simulate_preflop(const int c0, const int c1, const int iterations) const;
    const result simulate_flop(const int c0, const int c1, const int b0, const int b1, const int b2, const int iterations) const;
    const result simulate_turn(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3, const int iterations) const;
    const result simulate_river(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3, const int b4, const int iterations) const;

private:
    const twoplustwo_evaluator evaluator_;
};
