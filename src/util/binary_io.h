#pragma once

#include <cstdint>

template<class T>
void binary_read(std::istream& is, T& val)
{
    is.read(reinterpret_cast<char*>(&val), sizeof(T));
}

template<class T>
void binary_write(std::ostream& os, const T& val)
{
    os.write(reinterpret_cast<const char*>(&val), sizeof(T));
}

template<class T>
void binary_read(std::istream& is, std::vector<T>& cont)
{
    std::uint64_t size;
    binary_read(is, size);
    cont.resize(size);
    is.read(reinterpret_cast<char*>(&cont[0]), sizeof(T) * cont.size());
}

template<class T>
void binary_write(std::ostream& os, const std::vector<T>& cont)
{
    binary_write(os, std::uint64_t(cont.size()));
    os.write(reinterpret_cast<const char*>(&cont[0]), sizeof(T) * cont.size());
}

template<class T, std::size_t N>
void binary_read(std::istream& is, std::array<T, N>& arr)
{
    std::uint64_t size;
    binary_read(is, size);
    assert(size == N);
    is.read(reinterpret_cast<char*>(&arr[0]), sizeof(T) * arr.size());
}

template<class T, std::size_t N>
void binary_write(std::ostream& os, const std::array<T, N>& arr)
{
    binary_write(os, std::uint64_t(arr.size()));
    os.write(reinterpret_cast<const char*>(&arr[0]), sizeof(T) * arr.size());
}
