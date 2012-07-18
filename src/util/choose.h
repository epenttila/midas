#pragma once

#include <boost/math/special_functions/binomial.hpp>

template<class T>
const T choose(const T& n, const T& k)
{
    return n < k ? 0 : T(boost::math::binomial_coefficient<double>(n, k));
}
