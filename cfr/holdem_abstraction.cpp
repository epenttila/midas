#include "holdem_abstraction.h"
#include <fstream>
#include <iostream>
#include <map>
#include <omp.h>
#include <numeric>
#include <string>
#include <boost/tokenizer.hpp>
#include "card.h"
#include "holdem_preflop_lut.h"
#include "compare_and_swap.h"
#include "choose.h"
#include "k_means.h"
#include "holdem_game.h"

namespace
{
    typedef std::map<float, int> frequency_map;

    const frequency_map get_preflop_frequencies(const holdem_preflop_lut& lut)
    {
        frequency_map frequencies;

        for (int a = 0; a < 52; ++a)
        {
            for (int b = a + 1; b < 52; ++b)
                ++frequencies[lut.get(a, b).second]; // ehs2
        }

        return frequencies;
    }

    const frequency_map get_flop_frequencies(const holdem_flop_lut& lut)
    {
        frequency_map frequencies;

#pragma omp parallel
        {
            frequency_map thread_frequencies;

#pragma omp for schedule(dynamic)
            for (int i = 0; i < 52; ++i)
            {
                for (int j = i + 1; j < 52; ++j)
                {
                    for (int k = j + 1; k < 52; ++k)
                    {
                        for (int a = 0; a < 52; ++a)
                        {
                            if (a == i || a == j || a == k)
                                continue;

                            for (int b = a + 1; b < 52; ++b)
                            {
                                if (b == i || b == j || b == k)
                                    continue;

                                ++thread_frequencies[lut.get(a, b, i, j, k).second]; // ehs2
                            }
                        }
                    }
                }
            }

#pragma omp critical
            {
                for (auto i = thread_frequencies.begin(); i != thread_frequencies.end(); ++i)
                    frequencies[i->first] += i->second;
            }
        }

        return frequencies;
    }

    const frequency_map get_turn_frequencies(const holdem_turn_lut& lut)
    {
        frequency_map frequencies;

#pragma omp parallel
        {
            frequency_map thread_frequencies;

#pragma omp for schedule(dynamic)
            for (int i = 0; i < 52; ++i)
            {
                for (int j = i + 1; j < 52; ++j)
                {
                    for (int k = j + 1; k < 52; ++k)
                    {
                        for (int l = k + 1; l < 52; ++l)
                        {
                            for (int a = 0; a < 52; ++a)
                            {
                                if (a == i || a == j || a == k || a == l)
                                    continue;

                                for (int b = a + 1; b < 52; ++b)
                                {
                                    if (b == i || b == j || b == k || b == l)
                                        continue;

                                    ++thread_frequencies[lut.get(a, b, i, j, k, l).second]; // ehs2
                                }
                            }
                        }
                    }
                }
            }

#pragma omp critical
            {
                for (auto i = thread_frequencies.begin(); i != thread_frequencies.end(); ++i)
                    frequencies[i->first] += i->second;
            }
        }

        return frequencies;
    }

    const frequency_map get_river_frequencies(const holdem_river_lut& lut)
    {
        frequency_map frequencies;

#pragma omp parallel
        {
            frequency_map thread_frequencies;

#pragma omp for schedule(dynamic)
            for (int i = 0; i < 52; ++i)
            {
                for (int j = i + 1; j < 52; ++j)
                {
                    for (int k = j + 1; k < 52; ++k)
                    {
                        for (int l = k + 1; l < 52; ++l)
                        {
                            for (int m = l + 1; m < 52; ++m)
                            {
                                for (int a = 0; a < 52; ++a)
                                {
                                    if (a == i || a == j || a == k || a == l || a == m)
                                        continue;

                                    for (int b = a + 1; b < 52; ++b)
                                    {
                                        if (b == i || b == j || b == k || b == l || b == m)
                                            continue;

                                        ++thread_frequencies[lut.get(a, b, i, j, k, l, m)];
                                    }
                                }
                            }
                        }
                    }
                }
            }

#pragma omp critical
            {
                for (auto i = thread_frequencies.begin(); i != thread_frequencies.end(); ++i)
                    frequencies[i->first] += i->second;
            }
        }

        return frequencies;
    }

    const std::vector<float> get_percentiles(const frequency_map& frequencies, const int bucket_count)
    {
        const unsigned int total_hands = std::accumulate(frequencies.begin(), frequencies.end(), 0,
            [](int a, const frequency_map::value_type& v) { return a + v.second; });
        unsigned int cumulative = 0;
        std::vector<float> buckets;
        buckets.push_back(0);

        for (auto i = frequencies.begin(); i != frequencies.end(); ++i)
        {
            if (cumulative + i->second >= total_hands / bucket_count)
            {
                cumulative = 0;
                buckets.push_back(i->first);
            }
            else
            {
                cumulative += i->second;
            }
        }

        return buckets;
    }

    int get_percentile_bucket(const double value, const std::vector<float>& percentiles)
    {
        const auto it = std::upper_bound(percentiles.begin(), percentiles.end(), value);
        return int(std::distance(percentiles.begin(), it) - 1);
    }

    struct get_distance
    {
        template<class T>
        double operator()(const T& a, const T& b) const
        {
            double distance = 0;

            for (auto i = 0; i < a.size(); ++i)
                distance += (a[i] - b[i]) * (a[i] - b[i]);

            return distance;
        }
    };
}

holdem_abstraction::holdem_abstraction(const std::string& bucket_configuration)
    : evaluator_(new evaluator)
    , preflop_lut_(std::ifstream("holdem_preflop_lut.dat", std::ios::binary))
    , flop_lut_(std::ifstream("holdem_flop_lut.dat", std::ios::binary))
    , turn_lut_(std::ifstream("holdem_turn_lut.dat", std::ios::binary))
    , river_lut_(std::ifstream("holdem_river_lut.dat", std::ios::binary))
{
    boost::char_separator<char> sep(".");
    boost::tokenizer<boost::char_separator<char>> tok(bucket_configuration, sep);
    int round = 0;

    for (auto i = tok.begin(); i != tok.end(); ++i)
    {
        /// @todo implement percentile bucketing and imperfect recall
        /// @todo irN, dir-X-Y, hs2, hs
        bucket_cfgs_[round].type = bucket_cfg::PERFECT_RECALL;
        bucket_cfgs_[round].size = std::atoi(i->c_str());
        ++round;
    }

    if (bucket_cfgs_[holdem_game::PREFLOP].size < 169)
    {
        preflop_ehs2_percentiles_ = get_percentiles(get_preflop_frequencies(preflop_lut_),
            bucket_cfgs_[holdem_game::PREFLOP].size);
    }

    std::cout << "flop percentiles\n";

    flop_ehs2_percentiles_ = get_percentiles(get_flop_frequencies(flop_lut_), bucket_cfgs_[holdem_game::FLOP].size);

    std::cout << "turn percentiles\n";

    turn_ehs2_percentiles_ = get_percentiles(get_turn_frequencies(turn_lut_), bucket_cfgs_[holdem_game::TURN].size);

    std::cout << "river percentiles\n";

    river_ehs_percentiles_ = get_percentiles(get_river_frequencies(river_lut_), bucket_cfgs_[holdem_game::RIVER].size);

#if 0
    std::cout << "flops\n";

    std::vector<std::vector<double>> flops(22100, std::vector<double>(100));
    double t = omp_get_wtime();

#pragma omp parallel
    {
        std::vector<std::vector<double>> thread_flops = flops;

#pragma omp for schedule(dynamic)
        for (int i = 0; i < 52; ++i)
        {
            for (int j = i + 1; j < 52; ++j)
            {
                for (int k = j + 1; k < 52; ++k)
                {
                    const auto flop_key = choose(k, 3) + choose(j, 2) + choose(i, 1);

                    for (int l = 0; l < 52; ++l)
                    {
                        if (l == i || l == j || l == k)
                            continue;

                        for (int a = 0; a < 52; ++a)
                        {
                            if (a == i || a == j || a == k || a == l)
                                continue;

                            for (int b = a + 1; b < 52; ++b)
                            {
                                if (b == i || b == j || b == k || b == l)
                                    continue;

                                const int flop_bucket = get_private_flop_bucket(a, b, i, j, k);
                                const int turn_bucket = get_private_turn_bucket(a, b, i, j, k, l);
                                ++thread_flops[flop_key][flop_bucket * 10 + turn_bucket];
                            }
                        }
                    }
                }
            }
        }

#pragma omp critical
        {
            for (auto i = 0; i < flops.size(); ++i)
            {
                for (auto j = 0; j < flops[i].size(); ++j)
                    flops[i][j] += thread_flops[i][j];
            }
        }
    }

    std::cout << "time: " << omp_get_wtime() - t << "\n";
    std::ofstream ff("clusters.txt");

    for (auto i = 0; i < flops.size(); ++i)
    {
        for (auto j = 0; j < 100; ++j)
            ff << int(flops[i][j]) << " ";
        ff << "\n";
    }

    std::cout << "kmeans\n";
    public_flop_buckets_ = k_means<get_distance>(flops, 10, 1000);

    std::ofstream f("flops.txt");

    for (int i = 0; i < 52; ++i)
    {
        for (int j = i + 1; j < 52; ++j)
        {
            for (int k = j + 1; k < 52; ++k)
            {
                const auto flop_key = choose(k, 3) + choose(j, 2) + choose(i, 1);
                f << get_card_string(k) << " " << get_card_string(j) << " " << get_card_string(i) << " | " << public_flop_buckets_[flop_key] << "\n";
            }
        }
    }

    std::cout << "saved\n";
#endif
}

int holdem_abstraction::get_bucket_count(const int round) const
{
    int size = bucket_cfgs_[holdem_game::PREFLOP].size;

    if (round == holdem_game::PREFLOP)
        return size;

    size *= bucket_cfgs_[holdem_game::FLOP].size;

    if (round == holdem_game::FLOP)
        return size;

    size *= bucket_cfgs_[holdem_game::TURN].size;

    if (round == holdem_game::TURN)
        return size;

    size *= bucket_cfgs_[holdem_game::RIVER].size;
    return size;
}

void holdem_abstraction::get_buckets(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3,
    const int b4, bucket_type* buckets) const
{
    const int preflop_bucket = get_preflop_bucket(c0, c1);

    const int private_flop_bucket = get_private_flop_bucket(c0, c1, b0, b1, b2);
    const int flop_bucket = preflop_bucket * bucket_cfgs_[holdem_game::PREFLOP].size + private_flop_bucket;

    const int private_turn_bucket = get_private_turn_bucket(c0, c1, b0, b1, b2, b3);
    const int turn_bucket = flop_bucket * bucket_cfgs_[holdem_game::FLOP].size + private_turn_bucket;

    const int private_river_bucket = get_private_river_bucket(c0, c1, b0, b1, b2, b3, b4);
    const int river_bucket = turn_bucket * bucket_cfgs_[holdem_game::TURN].size + private_river_bucket;

#if 0
    {
        const auto private_bucket = get_private_flop_bucket(c0, c1, b0, b1, b2);

        std::array<int, 3> flop = {{b0, b1, b2}};
        /// @todo sort_board
        compare_and_swap(flop[1], flop[2]);
        compare_and_swap(flop[0], flop[2]);
        compare_and_swap(flop[0], flop[1]);

        const auto flop_key = choose(flop[2], 3) + choose(flop[1], 2) + choose(flop[0], 1);
        const auto public_bucket = public_flop_buckets_[flop_key];
    }
#endif


    (*buckets)[holdem_game::PREFLOP] = preflop_bucket;
    (*buckets)[holdem_game::FLOP] = flop_bucket;
    (*buckets)[holdem_game::TURN] = turn_bucket;
    (*buckets)[holdem_game::RIVER] = river_bucket;
}

int holdem_abstraction::get_preflop_bucket(const int c0, const int c1) const
{
    if (bucket_cfgs_[holdem_game::PREFLOP].size < 169)
        return get_percentile_bucket(preflop_lut_.get(c0, c1).second, preflop_ehs2_percentiles_);
    else
        return preflop_lut_.get_key(c0, c1);
}

int holdem_abstraction::get_private_flop_bucket(const int c0, const int c1, const int b0, const int b1,
    const int b2) const
{
    return get_percentile_bucket(flop_lut_.get(c0, c1, b0, b1, b2).second, flop_ehs2_percentiles_);
}

int holdem_abstraction::get_private_turn_bucket(const int c0, const int c1, const int b0, const int b1,
    const int b2, const int b3) const
{
    return get_percentile_bucket(turn_lut_.get(c0, c1, b0, b1, b2, b3).second, turn_ehs2_percentiles_);
}

int holdem_abstraction::get_private_river_bucket(const int c0, const int c1, const int b0, const int b1,
    const int b2, const int b3, const int b4) const
{
    return get_percentile_bucket(river_lut_.get(c0, c1, b0, b1, b2, b3, b4), river_ehs_percentiles_);
}
