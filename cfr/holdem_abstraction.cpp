#include "common.h"
#include "holdem_abstraction.h"
#include "holdem_evaluator.h"

const int holdem_abstraction::ITERATIONS = 10000;

holdem_abstraction::holdem_abstraction(const std::array<int, 4>&)
    : evaluator_(new holdem_evaluator)
{
}

int holdem_abstraction::get_preflop_bucket(const int c0, const int c1) const
{
    const std::pair<int, int> ranks = std::minmax(holdem_evaluator::get_rank(c0), holdem_evaluator::get_rank(c1));
    const int suits[2] = {holdem_evaluator::get_suit(c0), holdem_evaluator::get_suit(c1)};

    if (ranks.first == ranks.second)
        return 0;
    else if (suits[0] == suits[1])
        return 1;
    else
        return 2;
}

int holdem_abstraction::get_flop_bucket(const int c0, const int c1, const int b0, const int b1, const int b2) const
{
    return (evaluator_->get_hand_value(c0, c1, b0, b1, b2) >> 12) - 1;
}

int holdem_abstraction::get_turn_bucket(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3) const
{
    return (evaluator_->get_hand_value(c0, c1, b0, b1, b2, b3) >> 12) - 1;
}

int holdem_abstraction::get_river_bucket(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3, const int b4) const
{
    return (evaluator_->get_hand_value(c0, c1, b0, b1, b2, b3, b4) >> 12) - 1;
}

int holdem_abstraction::get_bucket_count(int round) const
{
    if (round == holdem_state::PREFLOP)
        return 3;
    else
        return 9;
}
