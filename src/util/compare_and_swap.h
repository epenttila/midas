#pragma once

template<class T>
void compare_and_swap(T& a, T& b)
{
    if (a > b)
        std::swap(a, b);
}
