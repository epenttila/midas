#pragma once

#include <boost/noncopyable.hpp>
#include <vector>
#include <string>

class holdem_evaluator : private boost::noncopyable
{
public:
    holdem_evaluator(const std::string& filename = "ranks.dat");
    int get_hand_value(int c0, int c1, int c2, int c3, int c4) const;
    int get_hand_value(int c0, int c1, int c2, int c3, int c4, int c5) const;
    int get_hand_value(int c0, int c1, int c2, int c3, int c4, int c5, int c6) const;
    double enumerate_river(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const;

private:
    std::vector<int> ranks_;
};
