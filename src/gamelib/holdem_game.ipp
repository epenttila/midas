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

    for (int i = 0; i < 52; ++i)
    {
        for (int j = 0; j < 52; ++j)
        {
            int index = -1;

            if (i != j)
            {
                assert(get_hole_index(i, j) == get_hole_index(j, i));
                index = get_hole_index(i, j);
                hole_cards_[index] = std::make_pair(j, i);
            }

            reverse_hole_cards_[i][j] = index;
            reverse_hole_cards_[j][i] = index;
        }
    }
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
void holdem_game<Abstraction>::play_public(buckets_type& buckets)
{
    partial_shuffle(deck_, 5, engine_);
    board_[0] = deck_[deck_.size() - 1];
    board_[1] = deck_[deck_.size() - 2];
    board_[2] = deck_[deck_.size() - 3];
    board_[3] = deck_[deck_.size() - 4];
    board_[4] = deck_[deck_.size() - 5];

    for (int i = 0; i < PRIVATE_OUTCOMES; ++i)
    {
        const int c0 = hole_cards_[i].first;
        const int c1 = hole_cards_[i].second;

        if (c0 == board_[0] || c0 == board_[1] || c0 == board_[2] || c0 == board_[3] || c0 == board_[4]
            || c1 == board_[0] || c1 == board_[1] || c1 == board_[2] || c1 == board_[3] || c1 == board_[4])
        {
            for (int j = 0; j < ROUNDS; ++j)
                buckets[j][i] = -1;
        }
        else
        {
            typename abstraction_t::bucket_type b;
            abstraction_.get_buckets(hole_cards_[i].first, hole_cards_[i].second, board_[0], board_[1], board_[2], board_[3], board_[4], &b);

            for (int j = 0; j < ROUNDS; ++j)
                buckets[j][i] = b[j];
        }
    }

    for (int i = 0; i < PRIVATE_OUTCOMES; ++i)
    {
        const int c0 = hole_cards_[i].first;
        const int c1 = hole_cards_[i].second;
        assert(c0 != c1);

        if (c0 == board_[0] || c0 == board_[1] || c0 == board_[2] || c0 == board_[3] || c0 == board_[4]
            || c1 == board_[0] || c1 == board_[1] || c1 == board_[2] || c1 == board_[3] || c1 == board_[4])
        {
            ranks_[i] = -1;
        }
        else
        {
            ranks_[i] = evaluator_.get_hand_value(c0, c1, board_[0], board_[1], board_[2], board_[3], board_[4]);
        }

        sorted_ranks_[i] = std::make_pair(ranks_[i], i);
    }

    std::sort(sorted_ranks_.begin(), sorted_ranks_.end());

    first_rank_ = 0;

    for (; first_rank_ < PRIVATE_OUTCOMES; ++first_rank_)
    {
        if (sorted_ranks_[first_rank_].first != -1)
            break;
    }

}

template<class Abstraction>
void holdem_game<Abstraction>::get_results(const int action, const reaches_type& reaches, results_type& results) const
{
    if (action == 0) // assume fold is 0
    {
		double opsum = 0;
        std::array<double, 52> cr = {{}};

		for (int i = first_rank_; i < PRIVATE_OUTCOMES; ++i)
		{
            const int ohole = sorted_ranks_[i].second;
			cr[hole_cards_[ohole].first] += reaches[ohole];
			cr[hole_cards_[ohole].second] += reaches[ohole];
			opsum += reaches[ohole];
		}

		for (int i = first_rank_; i < PRIVATE_OUTCOMES; ++i)
		{
            const int phole = sorted_ranks_[i].second;
			results[phole] = opsum - cr[hole_cards_[phole].first] - cr[hole_cards_[phole].second] + reaches[phole];
		}

#if !defined(NDEBUG)
        results_type test_results = {{}};

        for (int i = 0; i < PRIVATE_OUTCOMES; ++i)
        {
            if (ranks_[i] == -1)
                continue;

            for (int j = 0; j < PRIVATE_OUTCOMES; ++j)
            {
                if (i == j || ranks_[j] == -1)
                    continue;

                if (hole_cards_[i].first == hole_cards_[j].first
                    || hole_cards_[i].first == hole_cards_[j].second
                    || hole_cards_[i].second == hole_cards_[j].first
                    || hole_cards_[i].second == hole_cards_[j].second)
                {
                    continue;
                }

                test_results[i] += reaches[j];
            }

            assert(std::abs(results[i] - test_results[i]) < 0.00001);
        }
#endif
    }
    else
    {
        std::array<double, 52> wincr = {{}};
	    double winsum = 0;
	    int j = first_rank_;

	    for (int i = first_rank_; i < PRIVATE_OUTCOMES; ++i)
	    {
            const int phole = sorted_ranks_[i].second;

		    while (sorted_ranks_[j].first < sorted_ranks_[i].first)
		    {
                const int ohole = sorted_ranks_[j].second;
			    winsum += reaches[ohole];
			    wincr[hole_cards_[ohole].first] += reaches[ohole];
			    wincr[hole_cards_[ohole].second] += reaches[ohole];
			    ++j;
		    }

		    results[phole] = winsum - wincr[hole_cards_[phole].first] - wincr[hole_cards_[phole].second];
	    }

        std::array<double, 52> losecr = {{}};
	    double losesum = 0;
	    j = PRIVATE_OUTCOMES - 1;

	    for (int i = PRIVATE_OUTCOMES - 1; i >= first_rank_; --i)
	    {
            const int phole = sorted_ranks_[i].second;

		    while (sorted_ranks_[j].first > sorted_ranks_[i].first)
		    {
                const int ohole = sorted_ranks_[j].second;
			    losesum += reaches[ohole];
			    losecr[hole_cards_[ohole].first] += reaches[ohole];
			    losecr[hole_cards_[ohole].second] += reaches[ohole];
			    --j;
		    }

		    results[phole] -= losesum - losecr[hole_cards_[phole].first] - losecr[hole_cards_[phole].second];
	    }

#if !defined(NDEBUG)
        results_type test_results = {{}};

        for (int i = 0; i < PRIVATE_OUTCOMES; ++i)
        {
            if (ranks_[i] == -1)
                continue;

            for (int j = 0; j < PRIVATE_OUTCOMES; ++j)
            {
                if (i == j || ranks_[j] == -1)
                    continue;

                if (hole_cards_[i].first == hole_cards_[j].first
                    || hole_cards_[i].first == hole_cards_[j].second
                    || hole_cards_[i].second == hole_cards_[j].first
                    || hole_cards_[i].second == hole_cards_[j].second)
                {
                    continue;
                }

                if (ranks_[i] > ranks_[j])
                    test_results[i] += reaches[j];
                else if (ranks_[i] < ranks_[j])
                    test_results[i] -= reaches[j];
            }

            assert(std::abs(results[i] - test_results[i]) < 0.00001);
        }
#endif
    }
}

template<class Abstraction>
std::mt19937& holdem_game<Abstraction>::get_random_engine()
{
    return engine_;
}
