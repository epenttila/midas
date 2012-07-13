#pragma once

template<class T>
const T choose(const T& n, const T& k)
{
    if (n == k)
        return 1;
    else if (n < k)
        return 0;

    T start = n - k;
    T iterations = k;

    if (k > n - k)
        std::swap(start, iterations);

    T ans = 1;

    for (int i = 0; i < iterations; ++i)
        ans = ans * (start + i + 1) / (i + 1);

    return ans;
}
