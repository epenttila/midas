#include "strategy.h"
#include <cstdio>
#include <cstdint>
#include <cassert>
#include "cfrlib/game_state_base.h"

strategy::strategy(const std::string& filename, std::size_t states, bool read_only)
    : states_(states)
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

double strategy::get_probability(const game_state_base& state, int child, int bucket) const
{
    return *get_data(state, child, bucket);
}

const double* strategy::get_data(const game_state_base& state, int child, int bucket) const
{
    return reinterpret_cast<const double*>(file_.const_data() + get_position(state, child, bucket));
}

double* strategy::get_data(const game_state_base& state, int child, int bucket)
{
    return reinterpret_cast<double*>(file_.data() + get_position(state, child, bucket));
}

int strategy::get_random_child(const game_state_base& state, int bucket) const
{
    if (state.get_id() >= states_ || bucket < 0)
    {
        assert(false);
        return -1;
    }

    std::uniform_real_distribution<double> dist;
    double x = dist(engine_);

    for (int i = 0; i < state.get_child_count(); ++i)
    {
        const double p = get_probability(state, i, bucket);

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

std::size_t strategy::get_position(const game_state_base& state, int child, int bucket) const
{
    if (state.get_id() >= states_)
        throw std::runtime_error("invalid state id");

    if (child < 0 || child >= state.get_child_count())
        throw std::runtime_error("invalid child");
    
    if (bucket < 0)
        throw std::runtime_error("invalid bucket");

    return positions_[state.get_id()] + (bucket * state.get_child_count() + child) * sizeof(double);
}
