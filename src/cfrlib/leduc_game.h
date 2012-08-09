#pragma once

#include <random>
#include <array>
#include "abslib/leduc_abstraction.h"
#include "evallib/leduc_evaluator.h"

class leduc_game
{
public:
    static const int CARDS = 6;
    static const int PRIVATE_OUTCOMES = 6;
    static const int PUBLIC_CARDS = 1;

    enum leduc_round
    {
        PREFLOP,
        FLOP,
        ROUNDS
    };

    typedef std::array<std::array<int, ROUNDS>, 2> bucket_t;
    typedef leduc_evaluator evaluator_t;
    typedef leduc_abstraction abstraction_t;
    typedef std::array<int, PUBLIC_CARDS> public_type;
    typedef std::array<std::array<int, PRIVATE_OUTCOMES>, ROUNDS> buckets_type;

    struct result
    {
        double win;
        double tie;
        double lose;
    };

    typedef std::array<result, PRIVATE_OUTCOMES> results_type;
    typedef std::array<double, PRIVATE_OUTCOMES> reaches_type;

    leduc_game();
    int play(const evaluator_t& eval, const abstraction_t& abs, bucket_t* buckets);
    void play_public(const abstraction_t& abs, public_type& pub, buckets_type& buckets);
    static void get_results(const evaluator_t& eval, const public_type& pub, const reaches_type& reaches, results_type& results);

private:
    void get_public_sample(public_type& board);
    void get_buckets(const abstraction_t& abs, const public_type& pub, buckets_type& buckets) const;

    std::mt19937 engine_;
    std::array<int, CARDS> deck_;
};
