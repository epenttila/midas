#pragma once

#include <random>
#include "evallib/holdem_evaluator.h"
#include "abslib/holdem_abstraction.h"

class holdem_game
{
public:
    static const int PRIVATE_OUTCOMES = 1326;

    enum holdem_round
    {
        PREFLOP,
        FLOP,
        TURN,
        RIVER,
        ROUNDS
    };

    typedef std::array<std::array<int, ROUNDS>, 2> bucket_t;
    typedef holdem_evaluator evaluator_t;
    typedef holdem_abstraction abstraction_t;
    typedef std::array<int, 5> public_type;
    typedef std::array<std::array<int, PRIVATE_OUTCOMES>, ROUNDS> buckets_type;

    struct result
    {
        int win;
        int tie;
        int lose;
    };

    typedef std::array<result, PRIVATE_OUTCOMES> results_type;

    holdem_game();
    int play(const evaluator_t& eval, const abstraction_t& abs, bucket_t* buckets);
    void play_public(const evaluator_t& eval, const abstraction_t& abs, buckets_type& buckets, results_type& results);

private:
    void get_public_sample(public_type& board);
    void get_buckets(const abstraction_t& abs, const public_type& pub, buckets_type& buckets) const;
    void get_results(const evaluator_t& eval, const public_type& pub, results_type& results) const;

    std::mt19937 engine_;
    std::array<int, 52> deck_;
    std::array<std::pair<int, int>, PRIVATE_OUTCOMES> hole_cards_;
};
