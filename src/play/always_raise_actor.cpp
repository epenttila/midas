#include "always_raise_actor.h"
#include "nlhe_game.h"

void always_raise_actor::act(nlhe_game& g)
{
    g.raise(2);
}

void always_raise_actor::set_cards(int, int, int, int, int, int, int)
{
}

void always_raise_actor::opponent_called(const nlhe_game&)
{
}

void always_raise_actor::opponent_raised(const nlhe_game&, int)
{
}

void always_raise_actor::set_id(int)
{
}
