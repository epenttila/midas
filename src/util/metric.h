#pragma once

struct get_emd_distance
{
    template<class T>
    double operator()(const T& a, const T& b) const
    {
        assert(a.size() > 1 && b.size() > 1 && a.size() == b.size());

        double distance = 0;
        double prev = 0;

        for (std::size_t i = 1; i < a.size(); ++i)
        {
            const double cur = (a[i - 1] + prev) - b[i - 1];
            prev = cur;
            distance += std::abs(cur);
        }

        return distance;
    }
};

struct get_emd_cost
{
    template<class T>
    double operator()(const T& a, const T& b) const
    {
        const auto val = get_emd_distance()(a, b);
        return val * val;
    }
};

struct get_l2_distance
{
    template<class T>
    double operator()(const T& a, const T& b) const
    {
        assert(a.size() == b.size());

        double distance = 0;

        for (std::size_t i = 0; i < a.size(); ++i)
            distance += std::sqrt((a[i] - b[i]) * (a[i] - b[i]));

        return distance;
    }
};

struct get_l2_cost
{
    template<class T>
    double operator()(const T& a, const T& b) const
    {
        assert(a.size() == b.size());

        double distance = 0;

        for (std::size_t i = 0; i < a.size(); ++i)
            distance += (a[i] - b[i]) * (a[i] - b[i]);

        return distance;
    }
};
