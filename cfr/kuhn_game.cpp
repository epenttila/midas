#include "kuhn_game.h"
#include "partial_shuffle.h"

kuhn_game::kuhn_game()
{
    std::random_device rd;
    engine_.seed(rd());

    for (std::size_t i = 0; i < deck_.size(); ++i)
        deck_[i] = int(i);
}

int kuhn_game::play(const evaluator_t& eval, const abstraction_t& abs, bucket_t* buckets)
{
    partial_shuffle(deck_, 2, engine_);

    const int c0 = deck_[deck_.size() - 1];
    const int c1 = deck_[deck_.size() - 2];

    (*buckets)[0][kuhn_state::FIRST] = abs.get_bucket(c0);
    (*buckets)[1][kuhn_state::FIRST] = abs.get_bucket(c1);

    return eval.get_hand_value(c0) > eval.get_hand_value(c1) ? 1 : -1;
}
