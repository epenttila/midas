#include "always_fold_actor.h"
#include "nlhe_game.h"

void always_fold_actor::act(nlhe_game& g)
{
    g.fold();
}

void always_fold_actor::set_cards(int, int, int, int, int, int, int)
{
}

void always_fold_actor::opponent_called(const nlhe_game&)
{
}

void always_fold_actor::opponent_raised(const nlhe_game&, int)
{
}

void always_fold_actor::set_id(int)
{
}
