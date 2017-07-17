#include "leduc_dealer.h"
#include <numeric>
#include "util/partial_shuffle.h"
#include "util/choose.h"
#include "util/sort.h"

leduc_dealer::leduc_dealer(const evaluator_t& evaluator, const abstraction_t& abstraction, std::int64_t seed)
    : engine_(static_cast<unsigned long>(seed))
    , evaluator_(evaluator)
    , abstraction_(abstraction)
{
    deck_[0] = 0;
    deck_[1] = 1;
    deck_[2] = 2;
    deck_[3] = 3;
    deck_[4] = 4;
    deck_[5] = 5;
}

int leduc_dealer::play(bucket_t* buckets)
{
    partial_shuffle(deck_, 3, engine_); // 2 hole, 1 board

    std::array<int, 2> hands;

    hands[0] = deck_[deck_.size() - 1];
    hands[1] = deck_[deck_.size() - 2];
    const int board = deck_[deck_.size() - 3];

    std::array<int, 2> value;
       
    for (int k = 0; k < 2; ++k)
    {
        abstraction_.get_buckets(hands[k], board, &(*buckets)[k]);
        value[k] = evaluator_.get_hand_value(hands[k], board);
    }

    return value[0] > value[1] ? 1 : (value[0] < value[1] ? -1 : 0);
}

std::mt19937& leduc_dealer::get_random_engine()
{
    return engine_;
}
