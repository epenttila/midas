#pragma once

template<class T, class F>
void partial_shuffle(T& container, int shuffle_count, F& rand)
{
    typedef typename std::uniform_int_distribution<std::size_t> distr_t;
    typedef typename distr_t::param_type param_t;

    distr_t d;

    for (auto i = container.size() - 1; i >= container.size() - shuffle_count; --i)
        std::swap(container[i], container[d(rand, param_t(0, i))]);
}
