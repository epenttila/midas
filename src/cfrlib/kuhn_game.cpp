#include "kuhn_game.h"
#include "util/partial_shuffle.h"

kuhn_game::kuhn_game(const evaluator_t& evaluator, const abstraction_t& abstraction, std::int64_t seed)
    : evaluator_(evaluator)
    , abstraction_(abstraction)
    , engine_(static_cast<unsigned long>(seed))
{
    for (std::size_t i = 0; i < deck_.size(); ++i)
        deck_[i] = int(i);
}

int kuhn_game::play(bucket_t* buckets)
{
    partial_shuffle(deck_, 2, engine_);

    const int c0 = deck_[deck_.size() - 1];
    const int c1 = deck_[deck_.size() - 2];

    (*buckets)[0][FIRST] = abstraction_.get_bucket(c0);
    (*buckets)[1][FIRST] = abstraction_.get_bucket(c1);

    return evaluator_.get_hand_value(c0) > evaluator_.get_hand_value(c1) ? 1 : -1;
}

std::mt19937& kuhn_game::get_random_engine()
{
    return engine_;
}
