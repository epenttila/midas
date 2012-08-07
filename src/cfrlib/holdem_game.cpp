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

holdem_game::holdem_game()
{
    std::random_device rd;
    engine_.seed(rd());

    std::iota(deck_.begin(), deck_.end(), 0);

    for (int i = 0; i < 52; ++i)
    {
        for (int j = i + 1; j < 52; ++j)
        {
            if (i == j)
                continue;

            assert(get_hole_index(i, j) == get_hole_index(j, i));

            hole_cards_[get_hole_index(i, j)] = std::make_pair(j, i);
        }
    }
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

void holdem_game::play_public(const evaluator_t& eval, const abstraction_t& abs, buckets_type& buckets,
    results_type& results)
{
    public_type pub;
    get_public_sample(pub);
    get_buckets(abs, pub, buckets);
    get_results(eval, pub, results);
}

void holdem_game::get_public_sample(public_type& board)
{
    partial_shuffle(deck_, 5, engine_);
    board[0] = deck_[deck_.size() - 1];
    board[1] = deck_[deck_.size() - 2];
    board[2] = deck_[deck_.size() - 3];
    board[3] = deck_[deck_.size() - 4];
    board[4] = deck_[deck_.size() - 5];
}

void holdem_game::get_buckets(const abstraction_t& abs, const public_type& pub, buckets_type& buckets) const
{
    for (int i = 0; i < PRIVATE_OUTCOMES; ++i)
    {
        const int c0 = hole_cards_[i].first;
        const int c1 = hole_cards_[i].second;

        if (c0 == pub[0] || c0 == pub[1] || c0 == pub[2] || c0 == pub[3] || c0 == pub[4]
            || c1 == pub[0] || c1 == pub[1] || c1 == pub[2] || c1 == pub[3] || c1 == pub[4])
        {
            for (int j = 0; j < ROUNDS; ++j)
                buckets[j][i] = -1;
        }
        else
        {
            abstraction_t::bucket_type b;
            abs.get_buckets(hole_cards_[i].first, hole_cards_[i].second, pub[0], pub[1], pub[2], pub[3], pub[4], &b);

            for (int j = 0; j < ROUNDS; ++j)
                buckets[j][i] = b[j];
        }
    }
}

void holdem_game::get_results(const evaluator_t& eval, const public_type& pub, results_type& results) const
{
    std::array<int, PRIVATE_OUTCOMES> ranks;
    std::array<std::pair<int, int>, PRIVATE_OUTCOMES> sorted_ranks;

    for (int i = 0; i < PRIVATE_OUTCOMES; ++i)
    {
        const int c0 = hole_cards_[i].first;
        const int c1 = hole_cards_[i].second;
        assert(c0 != c1);

        if (c0 == pub[0] || c0 == pub[1] || c0 == pub[2] || c0 == pub[3] || c0 == pub[4]
            || c1 == pub[0] || c1 == pub[1] || c1 == pub[2] || c1 == pub[3] || c1 == pub[4])
        {
            ranks[i] = -1;
        }
        else
        {
            ranks[i] = eval.get_hand_value(c0, c1, pub[0], pub[1], pub[2], pub[3], pub[4]);
        }

        sorted_ranks[i] = std::make_pair(ranks[i], i);
    }

    std::sort(sorted_ranks.begin(), sorted_ranks.end());

    int invalid_rank_hole = 0;

    for (; invalid_rank_hole < PRIVATE_OUTCOMES; ++invalid_rank_hole)
    {
        if (sorted_ranks[invalid_rank_hole].first != -1)
            break;
    }

    int next_rank_hole = 0;
    int prev_rank_hole = 0;

    for (auto i = 0; i < sorted_ranks.size(); ++i)
    {
        const auto rank = sorted_ranks[i].first;
        const auto hole = sorted_ranks[i].second;

        if (rank == -1)
        {
            results[hole].win = -1;
            results[hole].tie = -1;
            results[hole].lose = -1;
            continue;
        }

        for (; prev_rank_hole < PRIVATE_OUTCOMES; ++prev_rank_hole)
        {
            if (sorted_ranks[prev_rank_hole].first == rank)
                break;
        }

        for (; next_rank_hole < PRIVATE_OUTCOMES; ++next_rank_hole)
        {
            if (sorted_ranks[next_rank_hole].first > rank)
                break;
        }
                
        int win = prev_rank_hole - invalid_rank_hole;
        int tie = next_rank_hole - prev_rank_hole;
        int lose = PRIVATE_OUTCOMES - next_rank_hole;

        const int c0 = hole_cards_[hole].first;
        const int c1 = hole_cards_[hole].second;

        for (int i = 0; i < 52; ++i)
        {
            const int x = c0 == i ? -1 : get_hole_index(c0, i);
            const int y = c1 == i ? -1 : get_hole_index(c1, i);

            if (x != -1 && ranks[x] != -1)
            {
                assert(x == get_hole_index(hole_cards_[x].first, hole_cards_[x].second));
            
                if (ranks[hole] > ranks[x])
                    --win;
                else if (ranks[hole] < ranks[x])
                    --lose;
                else
                    --tie;
            }

            if (y != -1 && ranks[y] != -1)
            {
                assert(y == get_hole_index(hole_cards_[y].first, hole_cards_[y].second));
    
                if (ranks[hole] > ranks[y])
                    --win;
                else if (ranks[hole] < ranks[y])
                    --lose;
                else
                    --tie;
            }
        }

        ++tie; // our cards (c0, c1) were subtracted twice, add one back

        assert(win >= 0 && win < PRIVATE_OUTCOMES);
        assert(tie >= 0 && tie < PRIVATE_OUTCOMES);
        assert(lose >= 0 && lose < PRIVATE_OUTCOMES);
        assert(win + tie + lose < PRIVATE_OUTCOMES);

        results[hole].win = win;
        results[hole].tie = tie;
        results[hole].lose = lose;
    }
}
