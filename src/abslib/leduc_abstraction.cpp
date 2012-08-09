#include "leduc_abstraction.h"

int leduc_abstraction::get_bucket(int card) const
{
    return card / 2; // ignore suit
}

int leduc_abstraction::get_bucket_count(int round) const
{
    if (round == 0)
        return 3; // Ja, Qa, Ka, Jb, Qb, Kb
    else
        return 3 * 3;
}

void leduc_abstraction::get_buckets(int card, int board, bucket_type* buckets) const
{
    (*buckets)[0] = get_bucket(card);
    (*buckets)[1] = (*buckets)[0] * 3 + get_bucket(board);
}
