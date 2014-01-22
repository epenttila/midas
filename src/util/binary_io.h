#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <array>

std::unique_ptr<FILE, int (*)(FILE*)> binary_open(const std::string& filename, const std::string& mode);

template<class T>
void binary_read(FILE& file, T& val)
{
    if (std::fread(&val, sizeof(T), 1, &file) != 1)
        throw std::runtime_error("binary_read(T) error");
}

template<class T>
void binary_write(FILE& file, const T& val)
{
    if (std::fwrite(&val, sizeof(T), 1, &file) != 1)
        throw std::runtime_error("binary_write(T) error");
}

template<class T>
void binary_read(FILE& file, std::vector<T>& cont)
{
    std::uint64_t size;
    binary_read(file, size);
    cont.resize(size);

    if (std::fread(&cont[0], sizeof(T), cont.size(), &file) != cont.size())
        throw std::runtime_error("binary_read(vector) error");
}

template<class T>
void binary_write(FILE& file, const std::vector<T>& cont)
{
    binary_write(file, std::uint64_t(cont.size()));

    if (std::fwrite(&cont[0], sizeof(T), cont.size(), &file) != cont.size())
        throw std::runtime_error("binary_write(vector) error");
}

template<class T, std::size_t N>
void binary_read(FILE& file, std::array<T, N>& arr)
{
    std::uint64_t size;
    binary_read(file, size);
    assert(size == N);

    if (std::fread(&arr[0], sizeof(T), arr.size(), &file) != arr.size())
        throw std::runtime_error("binary_read(array) error");
}

template<class T, std::size_t N>
void binary_write(FILE& file, const std::array<T, N>& arr)
{
    binary_write(file, std::uint64_t(arr.size()));

    if (std::fwrite(&arr[0], sizeof(T), arr.size(), &file) != arr.size())
        throw std::runtime_error("binary_write(array) error");
}

template<class T>
void binary_read(FILE& file, T* data, std::size_t count)
{
    if (std::fread(data, sizeof(T), count, &file) != count)
        throw std::runtime_error("binary_read(T*) error");
}

template<class T>
void binary_write(FILE& file, const T* data, std::size_t count)
{
    if (std::fwrite(data, sizeof(T), count, &file) != count)
        throw std::runtime_error("binary_write(T*) error");
}
