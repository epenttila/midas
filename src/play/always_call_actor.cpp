#include "always_call_actor.h"
#include "nlhe_game.h"

void always_call_actor::act(nlhe_game& g)
{
    g.call();
}

void always_call_actor::set_cards(int, int, int, int, int, int, int)
{
}

void always_call_actor::opponent_called(const nlhe_game&)
{
}

void always_call_actor::opponent_raised(const nlhe_game&, int)
{
}

void always_call_actor::set_id(int)
{
}
