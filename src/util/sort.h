#pragma once

template<class T>
void sort(T& a, T& b)
{
    if (a > b)
        std::swap(a, b);
}

template<class T>
void sort(std::array<T, 2>& data)
{
    sort(data[0], data[1]);
}

template<class T>
void sort(std::array<T, 3>& data)
{
    sort(data[1], data[2]);
    sort(data[0], data[2]);
    sort(data[0], data[1]);
}

template<class T>
void sort(std::array<T, 4>& data)
{
    sort(data[0], data[1]);
    sort(data[2], data[3]);
    sort(data[0], data[2]);
    sort(data[1], data[3]);
    sort(data[1], data[2]);
}

template<class T>
void sort(std::array<T, 5>& data)
{
    sort(data[0], data[1]);
    sort(data[3], data[4]);
    sort(data[2], data[4]);
    sort(data[2], data[3]);
    sort(data[0], data[3]);
    sort(data[0], data[2]);
    sort(data[1], data[4]);
    sort(data[1], data[3]);
    sort(data[1], data[2]);
}
