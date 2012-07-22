#pragma once

#include <array>
#include <vector>
#include <random>

class strategy
{
public:
    strategy(const std::string& filename, std::size_t states, int actions);
    double get(std::size_t state_id, int action, int bucket) const;
    int get_action(std::size_t state_id, int bucket) const;
    std::string get_filename() const;

private:
    std::size_t states_;
    int actions_;
    mutable std::mt19937 engine_;
    std::vector<std::size_t> positions_;
    std::unique_ptr<FILE, int (*)(FILE*)> file_;
    std::string filename_;
};
