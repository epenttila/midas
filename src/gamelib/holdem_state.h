#pragma once

#include "game_state_base.h"

class holdem_state : public game_state_base
{
public:
    enum game_round
    {
        INVALID_ROUND = -1,
        PREFLOP = 0,
        FLOP = 1,
        TURN = 2,
        RIVER = 3,
        ROUNDS,
    };
};
