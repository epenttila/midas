#pragma once

#include <array>
#include "gamelib/kuhn_state.h"

class kuhn_abstraction
{
public:
    int get_bucket(int card) const;
    int get_bucket_count(kuhn_state::game_round round) const;
};
