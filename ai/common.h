#ifdef _MSC_VER
#pragma warning(push, 3)
#endif
#define _SCL_SECURE_NO_WARNINGS
#define BOOST_DISABLE_ASSERTS
#include <array>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/functional/hash.hpp>
#include <boost/multi_array.hpp>
#include <cassert>
#include <cctype>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <random>
#include <tuple>
#include <unordered_map>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

template<class T, class F>
void partial_shuffle(T& container, int shuffle_count, F& rand)
{
    typedef typename std::uniform_int_distribution<std::size_t> distr_t;
    typedef typename distr_t::param_type param_t;

    distr_t d;

    for (auto i = container.size() - 1; i >= container.size() - shuffle_count; --i)
        std::swap(container[i], container[d(rand, param_t(0, i))]);
}
