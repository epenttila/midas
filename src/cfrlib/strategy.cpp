#include "strategy.h"
#include "util/binary_io.h"

strategy::strategy(std::size_t states, int actions)
    : data_(states * actions)
    , states_(states)
    , actions_(actions)
    , engine_(std::random_device()())
{
}

strategy::strategy(std::istream&& is)
    : engine_(std::random_device()())
{
    if (!is)
        throw std::runtime_error("bad istream");

    load(is);

    if (!is)
        throw std::runtime_error("read failed");
}

int strategy::get_actions() const
{
    return actions_;
}

void strategy::set(std::size_t state_id, int action, int bucket, double probability)
{
    data_[state_id * actions_ + action][bucket] = probability;
}

void strategy::set_buckets(std::size_t state_id, int action, int buckets)
{
    data_[state_id * actions_ + action].resize(buckets);
}

double strategy::get(std::size_t state_id, int action, int bucket) const
{
    const auto& values = data_[state_id * actions_ + action];
    
    if (bucket >= values.size())
        return -1;

    return values[bucket];
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

void strategy::save(std::ostream& os) const
{
    binary_write(os, states_);
    binary_write(os, actions_);

    for (std::size_t i = 0; i < data_.size(); ++i)
        binary_write(os, data_[i]);
}

void strategy::load(std::istream& is)
{
    binary_read(is, states_);
    binary_read(is, actions_);
    data_.resize(states_ * actions_);

    for (std::size_t i = 0; i < data_.size(); ++i)
        binary_read(is, data_[i]);
}
