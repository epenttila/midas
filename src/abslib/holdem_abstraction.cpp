#include "holdem_abstraction.h"
#include <fstream>
#include <iostream>
#include <map>
#include <omp.h>
#include <numeric>
#include <string>
#include <unordered_map>
#include "util/card.h"
#include "lutlib/holdem_preflop_lut.h"
#include "util/sort.h"
#include "util/choose.h"
#include "util/k_means.h"
#include "util/holdem_loops.h"
#include "util/binary_io.h"

namespace
{
    typedef std::map<float, int> frequency_map;
    typedef std::unordered_map<float, int> unordered_frequency_map;

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
        // TODO bins for percentiles (folly/histogram)
        unordered_frequency_map frequencies;

#pragma omp parallel
        {
            unordered_frequency_map thread_frequencies;

            parallel_for_each_flop([&](int a, int b, int i, int j, int k) {
                ++thread_frequencies[lut.get(a, b, i, j, k).second];
            });

#pragma omp critical
            {
                for (auto i = thread_frequencies.begin(); i != thread_frequencies.end(); ++i)
                    frequencies[i->first] += i->second;
            }
        }

        return frequency_map(frequencies.begin(), frequencies.end());
    }

    const frequency_map get_turn_frequencies(const holdem_turn_lut& lut)
    {
        unordered_frequency_map frequencies;

#pragma omp parallel
        {
            unordered_frequency_map thread_frequencies;

            parallel_for_each_turn([&](int a, int b, int i, int j, int k, int l) {
                ++thread_frequencies[lut.get(a, b, i, j, k, l).second];
            });

#pragma omp critical
            {
                for (auto i = thread_frequencies.begin(); i != thread_frequencies.end(); ++i)
                    frequencies[i->first] += i->second;
            }
        }

        return frequency_map(frequencies.begin(), frequencies.end());
    }

    const frequency_map get_river_frequencies(const holdem_river_lut& lut)
    {
        unordered_frequency_map frequencies;

#pragma omp parallel
        {
            unordered_frequency_map thread_frequencies;

            parallel_for_each_river([&](int a, int b, int i, int j, int k, int l, int m) {
                ++thread_frequencies[lut.get(a, b, i, j, k, l, m)];
            });

#pragma omp critical
            {
                for (auto i = thread_frequencies.begin(); i != thread_frequencies.end(); ++i)
                    frequencies[i->first] += i->second;
            }
        }

        return frequency_map(frequencies.begin(), frequencies.end());
    }

    const std::vector<float> get_percentiles(const frequency_map& frequencies, const int bucket_count)
    {
        const unsigned int total_hands = std::accumulate(frequencies.begin(), frequencies.end(), 0,
            [](int a, const frequency_map::value_type& v) { return a + v.second; });
        unsigned int cumulative = 0;
        const unsigned int bucket_size = total_hands / bucket_count;
        std::vector<float> buckets;
        buckets.push_back(0);

        for (auto i = frequencies.begin(); i != frequencies.end(); ++i)
        {
            cumulative += i->second;

            if (cumulative >= bucket_size)
            {
                buckets.push_back(i->first);
                cumulative -= bucket_size;
            }
        }

        buckets.resize(bucket_count);

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

    int index(int i, int j, int j_max)
    {
        return i * j_max + j;
    }

    int index(int i, int j, int j_max, int k, int k_max)
    {
        return i * j_max * k_max + j * k_max + k;
    }

    int index(int i, int j, int j_max, int k, int k_max, int l, int l_max)
    {
        return i * j_max * k_max * l_max + j * k_max * l_max + k * l_max + l;
    }

    int index(int i, int j, int j_max, int k, int k_max, int l, int l_max, int m, int m_max)
    {
        return i * j_max * k_max * l_max * m_max + j * k_max * l_max * m_max + k * l_max * m_max + l * m_max + m;
    }

    int index(int i, int j, int j_max, int k, int k_max, int l, int l_max, int m, int m_max, int n, int n_max)
    {
        return i * j_max * k_max * l_max * m_max * n_max
            + j * k_max * l_max * m_max * n_max
            + k * l_max * m_max * n_max
            + l * m_max * n_max
            + m * n_max
            + n;
    }

    std::vector<int> generate_public_flop_buckets(holdem_flop_lut& flop_lut, holdem_turn_lut& turn_lut,
        const int kmeans_max_iterations, const int kmeans_buckets, const int clusters)
    {
        std::vector<std::vector<double>> flops(choose(52, 3), std::vector<double>(kmeans_buckets * kmeans_buckets));
        const auto flop_percentiles = get_percentiles(get_flop_frequencies(flop_lut), kmeans_buckets);
        const auto turn_percentiles = get_percentiles(get_turn_frequencies(turn_lut), kmeans_buckets);

#pragma omp parallel
        {
            std::vector<std::vector<double>> thread_flops = flops;

            parallel_for_each_flop([&](int a, int b, int i, int j, int k) {
                const auto flop_key = choose(k, 3) + choose(j, 2) + choose(i, 1);

                for (int l = 0; l < 52; ++l)
                {
                    if (l == i || l == j || l == k || l == a || l == b)
                        continue;

                    const int flop_bucket =
                        get_percentile_bucket(flop_lut.get(a, b, i, j, k).second, flop_percentiles);
                    const int turn_bucket =
                        get_percentile_bucket(turn_lut.get(a, b, i, j, k, l).second, turn_percentiles);
                    ++thread_flops[flop_key][flop_bucket * kmeans_buckets + turn_bucket];
                }
            });

#pragma omp critical
            {
                for (auto i = 0; i < flops.size(); ++i)
                {
                    for (auto j = 0; j < flops[i].size(); ++j)
                        flops[i][j] += thread_flops[i][j];
                }
            }
        }

        return k_means<get_distance>(flops, clusters, kmeans_max_iterations);
    }

    std::vector<int> generate_public_turn_buckets(holdem_turn_lut& turn_lut, holdem_river_lut& river_lut,
        const int kmeans_max_iterations, const int kmeans_buckets, const int clusters)
    {
        std::vector<std::vector<double>> turns(choose(52, 4), std::vector<double>(kmeans_buckets * kmeans_buckets));
        const auto turn_percentiles = get_percentiles(get_turn_frequencies(turn_lut), kmeans_buckets);
        const auto river_percentiles = get_percentiles(get_river_frequencies(river_lut), kmeans_buckets);

#pragma omp parallel
        {
            std::vector<std::vector<double>> thread_turns = turns;

            parallel_for_each_turn([&](int a, int b, int i, int j, int k, int l) {
                const auto key = choose(l, 4) + choose(k, 3) + choose(j, 2) + choose(i, 1);

                for (int m = 0; m < 52; ++m)
                {
                    if (m == i || m == j || m == k || m == l || m == a || m == b)
                        continue;

                    const int from = get_percentile_bucket(turn_lut.get(a, b, i, j, k, l).second, turn_percentiles);
                    const int to = get_percentile_bucket(river_lut.get(a, b, i, j, k, l, m), river_percentiles);
                    ++thread_turns[key][from * kmeans_buckets + to];
                }
            });

#pragma omp critical
            {
                for (auto i = 0; i < turns.size(); ++i)
                {
                    for (auto j = 0; j < turns[i].size(); ++j)
                        turns[i][j] += thread_turns[i][j];
                }
            }
        }

        return k_means<get_distance>(turns, clusters, kmeans_max_iterations);
    }
}

holdem_abstraction::bucket_cfg::bucket_cfg()
    : hs2(1)
    , pub(1)
    , forget_hs2(false)
    , forget_pub(false)
{
}

holdem_abstraction::holdem_abstraction(const bucket_cfg_type& bucket_cfgs, int kmeans_max_iterations)
    : bucket_cfgs_(bucket_cfgs)
{
    init();

    preflop_ehs2_percentiles_.resize(bucket_cfgs_[PREFLOP].hs2);
    flop_ehs2_percentiles_.resize(bucket_cfgs_[FLOP].hs2);
    public_flop_buckets_.resize(bucket_cfgs_[FLOP].pub);
    turn_ehs2_percentiles_.resize(bucket_cfgs_[TURN].hs2);
    public_turn_buckets_.resize(bucket_cfgs_[TURN].pub);
    river_ehs_percentiles_.resize(bucket_cfgs_[RIVER].hs2);

    if (bucket_cfgs_[PREFLOP].hs2 > 1 && bucket_cfgs_[PREFLOP].hs2 < 169)
        preflop_ehs2_percentiles_ = get_percentiles(get_preflop_frequencies(*preflop_lut_), bucket_cfgs_[PREFLOP].hs2);

    if (bucket_cfgs_[FLOP].hs2 > 1)
        flop_ehs2_percentiles_ = get_percentiles(get_flop_frequencies(*flop_lut_), bucket_cfgs_[FLOP].hs2);

    if (bucket_cfgs_[TURN].hs2 > 1)
        turn_ehs2_percentiles_ = get_percentiles(get_turn_frequencies(*turn_lut_), bucket_cfgs_[TURN].hs2);

    if (bucket_cfgs_[RIVER].hs2 > 1)
        river_ehs_percentiles_ = get_percentiles(get_river_frequencies(*river_lut_), bucket_cfgs_[RIVER].hs2);
    
    if (bucket_cfgs_[FLOP].pub > 1)
    {
        public_flop_buckets_ = generate_public_flop_buckets(*flop_lut_, *turn_lut_, kmeans_max_iterations, 10,
            bucket_cfgs_[FLOP].pub);
    }

    if (bucket_cfgs_[TURN].pub > 1)
    {
        public_turn_buckets_ = generate_public_turn_buckets(*turn_lut_, *river_lut_, kmeans_max_iterations, 10,
            bucket_cfgs_[TURN].pub);
    }
}

holdem_abstraction::holdem_abstraction(const std::string& filename)
{
    std::ifstream is(filename, std::ios::binary);

    if (!is)
        throw std::runtime_error("bad istream");

    init();
    load(is);

    if (!is)
        throw std::runtime_error("read failed");
}

int holdem_abstraction::get_bucket_count(const int round) const
{
    const int remember_preflop =
        bucket_cfgs_[PREFLOP].forget_hs2 ? 1 : bucket_cfgs_[PREFLOP].hs2;
    const int remember_flop =
        (bucket_cfgs_[FLOP].forget_hs2 ? 1 : bucket_cfgs_[FLOP].hs2)
        * (bucket_cfgs_[FLOP].forget_pub ? 1 : bucket_cfgs_[FLOP].pub);
    const int remember_turn =
        (bucket_cfgs_[TURN].forget_hs2 ? 1 : bucket_cfgs_[TURN].hs2)
        * (bucket_cfgs_[TURN].forget_pub ? 1 : bucket_cfgs_[TURN].pub);

    switch (round)
    {
    case PREFLOP:
        return bucket_cfgs_[PREFLOP].hs2;

    case FLOP:
        return remember_preflop
            * bucket_cfgs_[FLOP].hs2
            * bucket_cfgs_[FLOP].pub;

    case TURN:
        return remember_preflop * remember_flop
            * bucket_cfgs_[TURN].hs2
            * bucket_cfgs_[TURN].pub;

    case RIVER:
        return remember_preflop * remember_flop * remember_turn
            * bucket_cfgs_[RIVER].hs2
            * bucket_cfgs_[RIVER].pub;
    }

    assert(false);
    return -1;
}

void holdem_abstraction::get_buckets(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3,
    const int b4, bucket_type* buckets) const
{
    assert(c0 != -1 && c1 != -1);

    const auto& cfgs = bucket_cfgs_;

    // PREFLOP
    int preflop_hs2 = get_preflop_bucket(c0, c1);
    int preflop_hs2_size = cfgs[PREFLOP].hs2;

    (*buckets)[PREFLOP] = preflop_hs2;

    if (cfgs[PREFLOP].forget_hs2)
    {
        preflop_hs2 = 0;
        preflop_hs2_size = 1;
    }

    // FLOP
    if (b0 == -1)
        return;

    int flop_hs2 = get_private_flop_bucket(c0, c1, b0, b1, b2);
    int flop_hs2_size = cfgs[FLOP].hs2;
    int flop_pub = get_public_flop_bucket(b0, b1, b2);
    int flop_pub_size = cfgs[FLOP].pub;

    (*buckets)[FLOP] = index(preflop_hs2, flop_hs2, flop_hs2_size, flop_pub, flop_pub_size);

    if (cfgs[FLOP].forget_hs2)
    {
        flop_hs2 = 0;
        flop_hs2_size = 1;
    }

    if (cfgs[FLOP].forget_pub)
    {
        flop_pub = 0;
        flop_pub_size = 1;
    }

    // TURN
    if (b3 == -1)
        return;

    int turn_hs2 = get_private_turn_bucket(c0, c1, b0, b1, b2, b3);
    int turn_hs2_size = cfgs[TURN].hs2;
    int turn_pub = get_public_turn_bucket(b0, b1, b2, b3);
    int turn_pub_size = cfgs[TURN].pub;

    (*buckets)[TURN] = index(preflop_hs2, flop_hs2, flop_hs2_size, flop_pub, flop_pub_size, turn_hs2, turn_hs2_size,
        turn_pub, turn_pub_size);

    if (cfgs[TURN].forget_hs2)
    {
        turn_hs2 = 0;
        turn_hs2_size = 1;
    }

    if (cfgs[TURN].forget_pub)
    {
        turn_pub = 0;
        turn_pub_size = 1;
    }

    // RIVER
    if (b4 == -1)
        return;

    const int river_hs = get_private_river_bucket(c0, c1, b0, b1, b2, b3, b4);
    const int river_hs_size = cfgs[RIVER].hs2;

    (*buckets)[RIVER] = index(preflop_hs2, flop_hs2, flop_hs2_size, flop_pub, flop_pub_size, turn_hs2, turn_hs2_size,
        turn_pub, turn_pub_size, river_hs, river_hs_size);
}

int holdem_abstraction::get_bucket(int c0, int c1) const
{
    bucket_type buckets;
    get_buckets(c0, c1, -1, -1, -1, -1, -1, &buckets);
    return buckets[PREFLOP];
}

int holdem_abstraction::get_bucket(int c0, int c1, int b0, int b1, int b2) const
{
    bucket_type buckets;
    get_buckets(c0, c1, b0, b1, b2, -1, -1, &buckets);
    return buckets[FLOP];
}

int holdem_abstraction::get_bucket(int c0, int c1, int b0, int b1, int b2, int b3) const
{
    bucket_type buckets;
    get_buckets(c0, c1, b0, b1, b2, b3, -1, &buckets);
    return buckets[TURN];
}

int holdem_abstraction::get_bucket(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const
{
    bucket_type buckets;
    get_buckets(c0, c1, b0, b1, b2, b3, b4, &buckets);
    return buckets[RIVER];
}

int holdem_abstraction::get_preflop_bucket(const int c0, const int c1) const
{
    if (bucket_cfgs_[PREFLOP].hs2 == 1)
        return 0;
    else if (bucket_cfgs_[PREFLOP].hs2 < 169)
        return get_percentile_bucket(preflop_lut_->get(c0, c1).second, preflop_ehs2_percentiles_);
    else
        return preflop_lut_->get_key(c0, c1);
}

int holdem_abstraction::get_private_flop_bucket(const int c0, const int c1, const int b0, const int b1,
    const int b2) const
{
    if (bucket_cfgs_[FLOP].hs2 > 1)
        return get_percentile_bucket(flop_lut_->get(c0, c1, b0, b1, b2).second, flop_ehs2_percentiles_);
    else
        return 0;
}

int holdem_abstraction::get_private_turn_bucket(const int c0, const int c1, const int b0, const int b1,
    const int b2, const int b3) const
{
    if (bucket_cfgs_[TURN].hs2 > 1)
        return get_percentile_bucket(turn_lut_->get(c0, c1, b0, b1, b2, b3).second, turn_ehs2_percentiles_);
    else
        return 0;
}

int holdem_abstraction::get_private_river_bucket(const int c0, const int c1, const int b0, const int b1,
    const int b2, const int b3, const int b4) const
{
    if (bucket_cfgs_[RIVER].hs2 > 1)
        return get_percentile_bucket(river_lut_->get(c0, c1, b0, b1, b2, b3, b4), river_ehs_percentiles_);
    else
        return 0;
}

int holdem_abstraction::get_public_flop_bucket(int b0, int b1, int b2) const
{
    if (bucket_cfgs_[FLOP].pub > 1)
    {
        std::array<int, 3> flop = {{b0, b1, b2}};
        sort(flop);

        const auto flop_key = choose(flop[2], 3) + choose(flop[1], 2) + choose(flop[0], 1);
        return public_flop_buckets_[flop_key];
    }
    else
    {
        return 0;
    }
}

int holdem_abstraction::get_public_turn_bucket(int b0, int b1, int b2, int b3) const
{
    if (bucket_cfgs_[TURN].pub > 1)
    {
        std::array<int, 4> board = {{b0, b1, b2, b3}};
        sort(board);

        const auto key = choose(board[3], 4) + choose(board[2], 3) + choose(board[1], 2) + choose(board[0], 1);
        return public_turn_buckets_[key];
    }
    else
    {
        return 0;
    }
}

void holdem_abstraction::save(std::ostream& os) const
{
    binary_write(os, bucket_cfgs_);
    binary_write(os, preflop_ehs2_percentiles_);
    binary_write(os, flop_ehs2_percentiles_);
    binary_write(os, turn_ehs2_percentiles_);
    binary_write(os, river_ehs_percentiles_);
    binary_write(os, public_flop_buckets_);
    binary_write(os, public_turn_buckets_);
}

void holdem_abstraction::load(std::istream& is)
{
    binary_read(is, bucket_cfgs_);
    binary_read(is, preflop_ehs2_percentiles_);
    binary_read(is, flop_ehs2_percentiles_);
    binary_read(is, turn_ehs2_percentiles_);
    binary_read(is, river_ehs_percentiles_);
    binary_read(is, public_flop_buckets_);
    binary_read(is, public_turn_buckets_);
}

void holdem_abstraction::init()
{
    evaluator_.reset(new evaluator);
    preflop_lut_.reset(new holdem_preflop_lut(std::ifstream("holdem_preflop_lut.dat", std::ios::binary)));
    flop_lut_.reset(new holdem_flop_lut(std::ifstream("holdem_flop_lut.dat", std::ios::binary)));
    turn_lut_.reset(new holdem_turn_lut(std::ifstream("holdem_turn_lut.dat", std::ios::binary)));
    river_lut_.reset(new holdem_river_lut(std::ifstream("holdem_river_lut.dat", std::ios::binary)));
}
