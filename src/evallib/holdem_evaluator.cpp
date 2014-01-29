#include "holdem_evaluator.h"
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <boost/log/trivial.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "util/binary_io.h"

holdem_evaluator::holdem_evaluator(const std::string& filename)
{
    BOOST_LOG_TRIVIAL(info) << "Loading ranks from: " << filename;

    auto f = binary_open(filename, "rb");
    
    if (!f)
        throw std::runtime_error("ranks.dat not found");

    ranks_.resize(32487834);
    binary_read(*f, ranks_.data(), ranks_.size());
}

int holdem_evaluator::get_hand_value(const int c0, const int c1, const int c2, const int c3, const int c4) const
{
    int p = ranks_[53 + c0 + 1];
    p = ranks_[p + c1 + 1];
    p = ranks_[p + c2 + 1];
    p = ranks_[p + c3 + 1];
    p = ranks_[p + c4 + 1];
    return ranks_[p];
}

int holdem_evaluator::get_hand_value(const int c0, const int c1, const int c2, const int c3, const int c4,
    const int c5) const
{
    int p = ranks_[53 + c0 + 1];
    p = ranks_[p + c1 + 1];
    p = ranks_[p + c2 + 1];
    p = ranks_[p + c3 + 1];
    p = ranks_[p + c4 + 1];
    p = ranks_[p + c5 + 1];
    return ranks_[p];
}

int holdem_evaluator::get_hand_value(const int c0, const int c1, const int c2, const int c3, const int c4,
    const int c5, const int c6) const
{
    int p = ranks_[53 + c0 + 1];
    p = ranks_[p + c1 + 1];
    p = ranks_[p + c2 + 1];
    p = ranks_[p + c3 + 1];
    p = ranks_[p + c4 + 1];
    p = ranks_[p + c5 + 1];
    return ranks_[p + c6 + 1];
}

double holdem_evaluator::enumerate_river(const int c0, const int c1, const int b0, const int b1, const int b2,
    const int b3, const int b4) const
{
    const int v0 = get_hand_value(c0, c1, b0, b1, b2, b3, b4);
    int showdowns = 0;
    int wins = 0;
    int ties = 0;

    for (int o0 = 0; o0 < 51; ++o0)
    {
        if (o0 == c0 || o0 == c1 || o0 == b0 || o0 == b1 || o0 == b2 || o0 == b3 || o0 == b4)
            continue;

        for (int o1 = o0 + 1; o1 < 52; ++o1)
        {
            if (o1 == c0 || o1 == c1 || o1 == b0 || o1 == b1 || o1 == b2 || o1 == b3 || o1 == b4)
                continue;

            const int v1 = get_hand_value(o0, o1, b0, b1, b2, b3, b4);

            if (v0 == v1)
                ++ties;
            else if (v0 > v1)
                ++wins;

            ++showdowns;
        }
    }

    return (wins + 0.5 * ties) / showdowns;
}
