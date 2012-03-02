#include "common.h"
#include "kuhn_abstraction.h"

kuhn_abstraction::kuhn_abstraction(const std::array<int, 1>&)
{
}

int kuhn_abstraction::get_bucket(int card) const
{
    return card; // no abstraction
}

int kuhn_abstraction::get_bucket_count(int) const
{
    return 3; // J, Q, K
}
