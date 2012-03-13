#include "holdem_game.h"
#include "holdem_abstraction.h"
#include "partial_shuffle.h"

holdem_game::holdem_game()
{
    std::random_device rd;
    engine_.seed(rd());

    for (std::size_t i = 0; i < deck_.size(); ++i)
        deck_[i] = int(i);
}

int holdem_game::play(const evaluator_t& eval, const abstraction_t& abs, bucket_t* buckets)
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
        abs.get_buckets(hands[k][0], hands[k][1], b0, b1, b2, b3, b4, &(*buckets)[k]);
        value[k] = eval.get_hand_value(hands[k][0], hands[k][1], b0, b1, b2, b3, b4);
    }

    return value[0] > value[1] ? 1 : (value[0] < value[1] ? -1 : 0);
}
