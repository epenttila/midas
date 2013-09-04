#pragma once

#include <random>
#include <cstdint>
#include "evallib/holdem_evaluator.h"
#include "abslib/holdem_abstraction.h"

class holdem_game : private boost::noncopyable
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
    typedef std::array<double, PRIVATE_OUTCOMES> results_type;
    typedef std::array<double, PRIVATE_OUTCOMES> reaches_type;

    holdem_game(const evaluator_t& eval, const abstraction_t& abs, std::int64_t seed);
    int play(bucket_t* buckets);
    void play_public(buckets_type& buckets);
    void get_results(int action, const reaches_type& reaches, results_type& results) const;
    std::mt19937& get_random_engine();

private:
    std::mt19937 engine_;
    std::array<int, 52> deck_;
    std::array<std::pair<int, int>, PRIVATE_OUTCOMES> hole_cards_;
    std::array<std::array<int, 52>, 52> reverse_hole_cards_;
    const evaluator_t& evaluator_;
    const abstraction_t& abstraction_;
    public_type board_;
    std::array<int, PRIVATE_OUTCOMES> ranks_;
    std::array<std::pair<int, int>, PRIVATE_OUTCOMES> sorted_ranks_;
    int first_rank_;
};
