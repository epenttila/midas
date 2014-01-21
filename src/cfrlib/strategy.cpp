#include "strategy.h"
#include <cstdio>
#include <cstdint>
#include <cassert>

strategy::strategy(const std::string& filename, std::size_t states, int actions, bool read_only)
    : states_(states)
    , actions_(actions)
    , engine_(std::random_device()())
    , positions_(states)
    , file_(filename, read_only ? boost::iostreams::mapped_file::readonly : boost::iostreams::mapped_file::readwrite)
    , filename_(filename)
{
    if (!file_)
        throw std::runtime_error("Unable to open strategy file");

    const auto pos = file_.size() - std::int64_t(states) * sizeof(positions_[0]);
    const auto p = reinterpret_cast<const std::uint64_t*>(file_.const_data() + pos);

    positions_.assign(p, p + positions_.size());
}

double strategy::get(std::size_t state_id, int action, int bucket) const
{
    return *get_data(state_id, action, bucket);
}

const double* strategy::get_data(std::size_t state_id, int action, int bucket) const
{
    return reinterpret_cast<const double*>(file_.const_data() + get_position(state_id, action, bucket));
}

double* strategy::get_data(std::size_t state_id, int action, int bucket)
{
    return reinterpret_cast<double*>(file_.data() + get_position(state_id, action, bucket));
}

int strategy::get_action(std::size_t state_id, int bucket) const
{
    if (state_id >= states_ || bucket < 0)
    {
        assert(false);
        return -1;
    }

    std::uniform_real_distribution<double> dist;
    double x = dist(engine_);

    for (int i = 0; i < actions_; ++i)
    {
        const double p = get(state_id, i, bucket);

        if (x < p && p > 0)
            return i;

        x -= p;
    }

    return -1;
}

std::string strategy::get_filename() const
{
    return filename_;
}

std::size_t strategy::get_position(std::size_t state_id, int action, int bucket) const
{
    if (state_id >= states_)
        throw std::runtime_error("invalid state id");

    if (action < 0 || action >= actions_)
        throw std::runtime_error("invalid action");
    
    if (bucket < 0)
        throw std::runtime_error("invalid bucket");

    return positions_[state_id] + (bucket * actions_ + action) * sizeof(double);
}
