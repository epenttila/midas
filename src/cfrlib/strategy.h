#pragma once

#include <array>
#include <vector>
#include <random>
#include <memory>
#include <boost/iostreams/device/mapped_file.hpp>

class strategy
{
public:
    strategy(const std::string& filename, std::size_t states, int actions, bool read_only = true);
    double get(std::size_t state_id, int action_index, int bucket) const;
    const double* get_data(std::size_t state_id, int action_index, int bucket) const;
    double* get_data(std::size_t state_id, int action_index, int bucket);
    int get_action(std::size_t state_id, int bucket) const;
    std::string get_filename() const;

private:
    std::size_t get_position(std::size_t state_id, int action, int bucket) const;

    std::size_t states_;
    int actions_;
    mutable std::mt19937 engine_;
    std::vector<std::size_t> positions_;
    boost::iostreams::mapped_file file_;
    std::string filename_;
};
