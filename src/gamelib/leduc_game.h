#pragma once

#include <random>
#include <array>
#include <cstdint>
#include <boost/noncopyable.hpp>
#include "abslib/leduc_abstraction.h"
#include "evallib/leduc_evaluator.h"
#include "util/game.h"

class leduc_game : private boost::noncopyable
{
public:
    static const int CARDS = 6;
    static const int PRIVATE_OUTCOMES = 6;
    static const int PUBLIC_CARDS = 1;
    static const int ROUNDS = FLOP + 1;

    typedef std::array<std::array<int, ROUNDS>, 2> bucket_t;
    typedef leduc_evaluator evaluator_t;
    typedef leduc_abstraction abstraction_t;
    typedef std::array<int, PUBLIC_CARDS> public_type;
    typedef std::array<std::array<int, PRIVATE_OUTCOMES>, ROUNDS> buckets_type;
    typedef std::array<double, PRIVATE_OUTCOMES> results_type;
    typedef std::array<double, PRIVATE_OUTCOMES> reaches_type;

    leduc_game(const evaluator_t& evaluator, const abstraction_t& abstraction, std::int64_t seed);
    int play(bucket_t* buckets);
    void play_public(buckets_type& buckets);
    void get_results(int action, const reaches_type& reaches, results_type& results) const;
    std::mt19937& get_random_engine();

private:
    std::mt19937 engine_;
    std::array<int, CARDS> deck_;
    const evaluator_t& evaluator_;
    const abstraction_t& abstraction_;
    public_type board_;
};
