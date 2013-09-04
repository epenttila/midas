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

leduc_game::leduc_game(const evaluator_t& evaluator, const abstraction_t& abstraction, std::int64_t seed)
    : evaluator_(evaluator)
    , abstraction_(abstraction)
    , engine_(static_cast<unsigned long>(seed))
{
    deck_[0] = 0;
    deck_[1] = 1;
    deck_[2] = 2;
    deck_[3] = 3;
    deck_[4] = 4;
    deck_[5] = 5;
}

int leduc_game::play(bucket_t* buckets)
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

void leduc_game::play_public(buckets_type& buckets)
{
    partial_shuffle(deck_, 1, engine_);
    board_[0] = deck_[deck_.size() - 1];

    for (int i = 0; i < PRIVATE_OUTCOMES; ++i)
    {
        abstraction_t::bucket_type b;
        abstraction_.get_buckets(i, board_[0], &b);

        for (int j = 0; j < ROUNDS; ++j)
            buckets[j][i] = b[j];
    }
}

void leduc_game::get_results(const int action, const reaches_type& reaches, results_type& results) const
{
    for (int i = 0; i < PRIVATE_OUTCOMES; ++i)
    {
        results[i] = 0;

        if (i == board_[0])
            continue;

        for (int j = 0; j < PRIVATE_OUTCOMES; ++j)
        {
            if (j == i || j == board_[0])
                continue;

            if (action == 0)
            {
                results[i] += reaches[j];
            }
            else
            {
                if (evaluator_.get_hand_value(i, board_[0]) > evaluator_.get_hand_value(j, board_[0]))
                    results[i] += reaches[j];
                else if (evaluator_.get_hand_value(i, board_[0]) < evaluator_.get_hand_value(j, board_[0]))
                    results[i] -= reaches[j];
            }
        }
    }
}

std::mt19937& leduc_game::get_random_engine()
{
    return engine_;
}
