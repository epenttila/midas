#include "holdem_game.h"
#include <numeric>
#include "util/partial_shuffle.h"
#include "util/choose.h"
#include "util/sort.h"

namespace
{
    int get_hole_index(int c0, int c1)
    {
        assert(c0 != c1);
        sort(c0, c1);
        return choose(c1, 2) + choose(c0, 1);
    }
}

template<class Abstraction>
holdem_game<Abstraction>::holdem_game(const evaluator_t& evaluator, const abstraction_t& abstraction, std::int64_t seed)
    : engine_(static_cast<unsigned long>(seed))
    , evaluator_(evaluator)
    , abstraction_(abstraction)
{
    std::iota(deck_.begin(), deck_.end(), 0);
}

template<class Abstraction>
int holdem_game<Abstraction>::play(bucket_t* buckets)
{
    partial_shuffle(deck_, 9, engine_); // 4 hole, 5 board

    std::array<std::array<int, 2>, 2> hands;

    hands[0][0] = deck_[deck_.size() - 1];
    hands[0][1] = deck_[deck_.size() - 2];
    hands[1][0] = deck_[deck_.size() - 3];
    hands[1][1] = deck_[deck_.size() - 4];
    const int b0 = deck_[deck_.size() - 5];
    const int b1 = deck_[deck_.size() - 6];
    const int b2 = deck_[deck_.size() - 7];
    const int b3 = deck_[deck_.size() - 8];
    const int b4 = deck_[deck_.size() - 9];

    std::array<int, 2> value;
       
    for (int k = 0; k < 2; ++k)
    {
        abstraction_.get_buckets(hands[k][0], hands[k][1], b0, b1, b2, b3, b4, &(*buckets)[k]);
        value[k] = evaluator_.get_hand_value(hands[k][0], hands[k][1], b0, b1, b2, b3, b4);
    }

    return value[0] > value[1] ? 1 : (value[0] < value[1] ? -1 : 0);
}

template<class Abstraction>
std::mt19937& holdem_game<Abstraction>::get_random_engine()
{
    return engine_;
}
