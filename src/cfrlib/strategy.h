#pragma once

#include <array>
#include <vector>
#include <random>
#include <memory>
#include <boost/iostreams/device/mapped_file.hpp>

class game_state_base;

class strategy
{
public:
    typedef float probability_t;

    strategy(const std::string& filename, int states, bool read_only = true);
    probability_t get_probability(const game_state_base& state, int child, int bucket) const;
    const probability_t* get_data(const game_state_base& state, int child, int bucket) const;
    probability_t* get_data(const game_state_base& state, int child, int bucket);
    int get_random_child(const game_state_base& state, int bucket) const;
    std::string get_filename() const;

private:
    std::size_t get_position(const game_state_base& state, int child, int bucket) const;

    int states_;
    mutable std::mt19937 engine_;
    std::vector<std::size_t> positions_;
    boost::iostreams::mapped_file file_;
    std::string filename_;
};
