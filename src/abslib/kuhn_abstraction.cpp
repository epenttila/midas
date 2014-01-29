#include "kuhn_abstraction.h"

int kuhn_abstraction::get_bucket(int card) const
{
    return card; // no abstraction
}

int kuhn_abstraction::get_bucket_count(kuhn_state::game_round) const
{
    return 3; // J, Q, K
}
