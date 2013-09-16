#pragma once

#include <string>
#include <boost/format.hpp>

enum
{
    PREFLOP,
    FLOP,
    TURN,
    RIVER,
};

static const int ALLIN_BET_SIZE = 999;

inline std::string format_money(double x)
{
    std::string s = (boost::format("$%1$.2f") % x).str();

    if (s[s.size() - 1] == '0' && s[s.size() - 2] == '0')
        s = s.substr(0, s.find('.'));

    return s;
}
