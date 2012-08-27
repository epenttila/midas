#pragma once

#include "actor_base.h"

class nlhe_game;

class always_call_actor : public actor_base
{
public:
    void act(nlhe_game& g);
    void set_cards(int, int, int, int, int, int, int);
    void opponent_called(const nlhe_game&);
    void opponent_raised(const nlhe_game&, int);
    void set_id(int);
};
