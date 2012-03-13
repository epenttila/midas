#include "holdem_abstraction.h"
#include "card.h"

holdem_abstraction::holdem_abstraction(const bucket_type&)
    : evaluator_(new evaluator)
{
}

int holdem_abstraction::get_preflop_bucket(const int c0, const int c1) const
{
    const std::pair<int, int> ranks = std::minmax(get_rank(c0), get_rank(c1));
    const int suits[2] = {get_suit(c0), get_suit(c1)};

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

void holdem_abstraction::get_buckets(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3,
    const int b4, bucket_type* buckets) const
{
    (*buckets)[holdem_state::PREFLOP] = get_preflop_bucket(c0, c1);
    (*buckets)[holdem_state::FLOP] = get_flop_bucket(c0, c1, b0, b1, b2);
    (*buckets)[holdem_state::TURN] = get_turn_bucket(c0, c1, b0, b1, b2, b3);
    (*buckets)[holdem_state::RIVER] = get_river_bucket(c0, c1, b0, b1, b2, b3, b4);
}
