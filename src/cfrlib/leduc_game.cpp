#include "leduc_game.h"
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

leduc_game::leduc_game()
{
    std::random_device rd;
    engine_.seed(rd());

    deck_[0] = 0;
    deck_[1] = 1;
    deck_[2] = 2;
    deck_[3] = 3;
    deck_[4] = 4;
    deck_[5] = 5;
}

int leduc_game::play(const evaluator_t& eval, const abstraction_t& abs, bucket_t* buckets)
{
    partial_shuffle(deck_, 3, engine_); // 2 hole, 1 board

    std::array<int, 2> hands;

    hands[0] = deck_[deck_.size() - 1];
    hands[1] = deck_[deck_.size() - 2];
    const int board = deck_[deck_.size() - 3];

    std::array<int, 2> value;
       
    for (int k = 0; k < 2; ++k)
    {
        abs.get_buckets(hands[k], board, &(*buckets)[k]);
        value[k] = eval.get_hand_value(hands[k], board);
    }

    return value[0] > value[1] ? 1 : (value[0] < value[1] ? -1 : 0);
}

void leduc_game::play_public(const abstraction_t& abs, public_type& pub, buckets_type& buckets)
{
    get_public_sample(pub);
    get_buckets(abs, pub, buckets);
}

void leduc_game::get_public_sample(public_type& board)
{
    partial_shuffle(deck_, 1, engine_);
    board[0] = deck_[deck_.size() - 1];
}

void leduc_game::get_buckets(const abstraction_t& abs, const public_type& pub, buckets_type& buckets) const
{
    for (int i = 0; i < PRIVATE_OUTCOMES; ++i)
    {
        abstraction_t::bucket_type b;
        abs.get_buckets(i, pub[0], &b);

        for (int j = 0; j < ROUNDS; ++j)
            buckets[j][i] = b[j];
    }
}

void leduc_game::get_results(const evaluator_t& eval, const public_type& pub, const reaches_type& reaches, results_type& results)
{
    for (int i = 0; i < PRIVATE_OUTCOMES; ++i)
    {
        results[i].win = 0;
        results[i].tie = 0;
        results[i].lose = 0;

        if (i == pub[0])
            continue;

        for (int j = 0; j < PRIVATE_OUTCOMES; ++j)
        {
            if (j == i || j == pub[0])
                continue;

            if (eval.get_hand_value(i, pub[0]) > eval.get_hand_value(j, pub[0]))
                results[i].win += reaches[j];
            else if (eval.get_hand_value(i, pub[0]) < eval.get_hand_value(j, pub[0]))
                results[i].lose += reaches[j];
            else
                results[i].tie += reaches[j];
        }
    }
}
