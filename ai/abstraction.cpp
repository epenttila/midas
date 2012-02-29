#include "common.h"
#include "abstraction.h"
#include "evaluator.h"

const int abstraction::ITERATIONS = 10000;

abstraction::abstraction(const bucket_count_t& bucket_counts)
    : bucket_counts_(bucket_counts)
    , evaluator_(new evaluator)
{
    assert(bucket_counts[holdem_state::PREFLOP] == 3);
    assert(bucket_counts[holdem_state::FLOP] == 9);
    assert(bucket_counts[holdem_state::TURN] == 9);
    assert(bucket_counts[holdem_state::RIVER] == 9);
}

int abstraction::get_preflop_bucket(const int c0, const int c1) const
{
    const std::pair<int, int> ranks = std::minmax(evaluator::get_rank(c0), evaluator::get_rank(c1));
    const int suits[2] = {evaluator::get_suit(c0), evaluator::get_suit(c1)};

    if (ranks.first == ranks.second)
        return 0;
    else if (suits[0] == suits[1])
        return 1;
    else
        return 2;
}

int abstraction::get_flop_bucket(const int c0, const int c1, const int b0, const int b1, const int b2) const
{
    return (evaluator_->get_hand_value(c0, c1, b0, b1, b2) >> 12) - 1;
}

int abstraction::get_turn_bucket(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3) const
{
    return (evaluator_->get_hand_value(c0, c1, b0, b1, b2, b3) >> 12) - 1;
}

int abstraction::get_river_bucket(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3, const int b4) const
{
    return (evaluator_->get_hand_value(c0, c1, b0, b1, b2, b3, b4) >> 12) - 1;
}
