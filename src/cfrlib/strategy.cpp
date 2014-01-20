#include "strategy.h"
#include <cstdio>
#include <cstdint>
#include <cassert>

strategy::strategy(const std::string& filename, std::size_t states, int actions)
    : states_(states)
    , actions_(actions)
    , engine_(std::random_device()())
    , positions_(states)
    , file_(filename)
    , filename_(filename)
{
    if (!file_)
        throw std::runtime_error("Unable to open strategy file");

    const auto pos = file_.size() - std::int64_t(states) * sizeof(positions_[0]);
    const auto p = reinterpret_cast<const std::uint64_t*>(file_.data() + pos);

    positions_.assign(p, p + positions_.size());
}

double strategy::get(std::size_t state_id, int action, int bucket) const
{
    if (state_id >= states_ || action < 0 || action >= actions_ || bucket < 0)
    {
        assert(false);
        return 0;
    }

    const auto pos = positions_[state_id] + (bucket * actions_ + action) * sizeof(double);
    const auto p = reinterpret_cast<const double*>(file_.data() + pos);

    return *p;
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
