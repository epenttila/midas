#pragma once

#include <random>
#include <boost/algorithm/clamp.hpp>

template<class T>
double get_normal_random(T& engine, double min, double max)
{
    const auto mean = (min + max) / 2.0;
    const auto sigma = (mean - min) / 3.0; // 99.7% within 3 sigma

    const auto x = std::normal_distribution<>(mean, sigma)(engine);

    return boost::algorithm::clamp(x, min, max);
}

template<class T>
double get_uniform_random(T& engine, double min, double max)
{
    const auto x = std::uniform_real_distribution<>(min, max)(engine);

    return boost::algorithm::clamp(x, min, max);
}
