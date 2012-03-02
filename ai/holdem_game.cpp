#include "common.h"
#include "holdem_game.h"
#include "holdem_abstraction.h"
#include "holdem_evaluator.h"

holdem_game::holdem_game()
{
    std::random_device rd;
    engine_.seed(rd());

    for (std::size_t i = 0; i < deck_.size(); ++i)
        deck_[i] = int(i);
}

int holdem_game::play(const evaluator_t& eval, const abstraction_t& abs, bucket_t* buckets)
{
    //assert(states_.size() == 6378);

    partial_shuffle(deck_, 9, engine_); // 4 hole, 5 board

    std::array<std::array<int, 7>, 2> hand;
    hand[0][0] = deck_[deck_.size() - 1];
    hand[0][1] = deck_[deck_.size() - 2];
    hand[1][0] = deck_[deck_.size() - 3];
    hand[1][1] = deck_[deck_.size() - 4];
    hand[0][2] = hand[1][2] = deck_[deck_.size() - 5];
    hand[0][3] = hand[1][3] = deck_[deck_.size() - 6];
    hand[0][4] = hand[1][4] = deck_[deck_.size() - 7];
    hand[0][5] = hand[1][5] = deck_[deck_.size() - 8];
    hand[0][6] = hand[1][6] = deck_[deck_.size() - 9];
       
    for (int k = 0; k < 2; ++k)
    {
        (*buckets)[k][holdem_state::PREFLOP] = abs.get_preflop_bucket(hand[k][0], hand[k][1]);
        (*buckets)[k][holdem_state::FLOP] = abs.get_flop_bucket(hand[k][0], hand[k][1], hand[k][2], hand[k][3],
            hand[k][4]);
        (*buckets)[k][holdem_state::TURN] = abs.get_turn_bucket(hand[k][0], hand[k][1], hand[k][2], hand[k][3],
            hand[k][4], hand[k][5]);
        (*buckets)[k][holdem_state::RIVER] = abs.get_river_bucket(hand[k][0], hand[k][1], hand[k][2], hand[k][3],
            hand[k][4], hand[k][5], hand[k][6]);
    }

    const int val1 = eval.get_hand_value(hand[0][0], hand[0][1], hand[0][2], hand[0][3], hand[0][4], hand[0][5],
        hand[0][6]);
    const int val2 = eval.get_hand_value(hand[1][0], hand[1][1], hand[1][2], hand[1][3], hand[1][4], hand[1][5],
        hand[1][6]);
    return val1 > val2 ? 1 : (val1 < val2 ? -1 : 0);
}
