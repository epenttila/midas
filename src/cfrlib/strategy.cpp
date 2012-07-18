#define _CRT_SECURE_NO_WARNINGS
#include "strategy.h"
#include <cstdio>
#include <cstdint>

strategy::strategy(const std::string& filename, std::size_t states, int actions)
    : states_(states)
    , actions_(actions)
    , engine_(std::random_device()())
    , positions_(states)
    , file_(fopen(filename.c_str(), "rb"), fclose)
{
    _fseeki64(file_.get(), -std::int64_t(states) * sizeof(positions_[0]), SEEK_END);
    fread(&positions_[0], sizeof(positions_[0]), positions_.size(), file_.get());
}

double strategy::get(std::size_t state_id, int action, int bucket) const
{
    std::size_t pos = (positions_[state_id] + bucket * actions_ + action) * sizeof(double);
    _fseeki64(file_.get(), pos, SEEK_SET);
    double value;
    fread(&value, sizeof(value), 1, file_.get());
    return value;
}

int strategy::get_action(std::size_t state_id, int bucket) const
{
    std::uniform_real_distribution<double> dist;
    double x = dist(engine_);

    for (int i = 0; i < actions_; ++i)
    {
        const double p = get(state_id, i, bucket);

        if (x < p)
            return i;

        x -= p;
    }

    return -1;
}
