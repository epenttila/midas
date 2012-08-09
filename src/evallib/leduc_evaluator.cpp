#include "leduc_evaluator.h"

int leduc_evaluator::get_hand_value(int card, int board) const
{
    // JX = 0, QX = 1, KX = 2
    // JJ = 3, QQ = 4, KK = 5
    const int rank = card / 2;

    if (rank == board / 2)
        return 3 + rank;
    else
        return rank;
}
