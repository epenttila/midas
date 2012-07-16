#pragma once

#include <array>
#include <vector>
#include <random>

class strategy
{
public:
    strategy(std::size_t states, int actions);
    strategy(std::istream&& is);
    int get_actions() const;
    void set(std::size_t state_id, int action, int bucket, double probability);
    void set_buckets(std::size_t state_id, int action, int buckets);
    double get(std::size_t state_id, int action, int bucket) const;
    int get_action(std::size_t state_id, int bucket) const;
    void save(std::ostream& os) const;
    void load(std::istream& is);

private:
    std::vector<std::vector<double>> data_;
    std::size_t states_;
    int actions_;
    mutable std::mt19937 engine_;
};
