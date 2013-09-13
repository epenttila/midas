#include "holdem_abstraction_v2.h"
#include <fstream>
#include <omp.h>
#include <numeric>
#include <boost/assign/list_of.hpp>
#include "util/k_means.h"
#include "util/binary_io.h"
#include "lutlib/hand_indexer.h"
#include "util/metric.h"

namespace
{
    typedef holdem_abstraction_v2::bucket_idx_t bucket_idx_t;

    template<int Dimensions>
    std::vector<bucket_idx_t> create_preflop_buckets(const hand_indexer& indexer, const holdem_river_lut& river_lut,
        int cluster_count, int kmeans_max_iterations)
    {
        typedef std::array<float, Dimensions> point_t;
        std::vector<point_t> data_points(indexer.get_size(indexer.get_rounds() - 1));

#pragma omp parallel for
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(data_points.size()); ++i)
        {
            auto& p = data_points[i];
            std::fill(p.begin(), p.end(), point_t::value_type());

            std::array<uint8_t, 2> cards;
            indexer.hand_unindex(indexer.get_rounds() - 1, i, cards.data());

            const auto c0 = cards[0];
            const auto c1 = cards[1];

            for (int b0 = 0; b0 < 48; ++b0)
            {
                if (b0 == c0 || b0 == c1)
                    continue;

                for (int b1 = b0 + 1; b1 < 49; ++b1)
                {
                    if (b1 == c0 || b1 == c1)
                        continue;

                    for (int b2 = b1 + 1; b2 < 50; ++b2)
                    {
                        if (b2 == c0 || b2 == c1)
                            continue;

                        for (int b3 = b2 + 1; b3 < 51; ++b3)
                        {
                            if (b3 == c0 || b3 == c1)
                                continue;

                            for (int b4 = b3 + 1; b4 < 52; ++b4)
                            {
                                if (b4 == c0 || b4 == c1)
                                    continue;

                                const auto hs = river_lut.get(c0, cards[1], b0, b1, b2, b3, b4);
                                const auto bin = std::min(std::size_t(hs * p.size()), p.size() - 1);
                                ++data_points[i][bin];
                            }
                        }
                    }
                }
            }

            const auto sum = std::accumulate(p.begin(), p.end(), 0);

            for (auto it = p.begin(); it != p.end(); ++it)
                *it /= sum;
        }

        std::vector<bucket_idx_t> buckets;
        std::vector<point_t> centers;

        k_means<point_t, bucket_idx_t, get_emd_distance, get_emd_cost>()(data_points, cluster_count,
            kmeans_max_iterations, OPTIMAL, &buckets, &centers);

        return buckets;
    }

    template<int Dimensions>
    std::vector<bucket_idx_t> create_flop_buckets(const hand_indexer& indexer, const holdem_river_lut& river_lut,
        int cluster_count, int kmeans_max_iterations)
    {
        typedef std::array<float, Dimensions> point_t;
        std::vector<point_t> data_points(indexer.get_size(indexer.get_rounds() - 1));

#pragma omp parallel for
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(data_points.size()); ++i)
        {
            auto& p = data_points[i];
            std::fill(p.begin(), p.end(), point_t::value_type());

            std::array<uint8_t, 5> cards;
            indexer.hand_unindex(indexer.get_rounds() - 1, i, cards.data());

            const auto c0 = cards[0];
            const auto c1 = cards[1];
            const auto b0 = cards[2];
            const auto b1 = cards[3];
            const auto b2 = cards[4];

            for (int b3 = 0; b3 < 51; ++b3)
            {
                if (b3 == c0 || b3 == c1 || b3 == b0 || b3 == b1 || b3 == b2)
                    continue;

                for (int b4 = b3 + 1; b4 < 52; ++b4)
                {
                    if (b4 == c0 || b4 == c1 || b4 == b0 || b4 == b1 || b4 == b2)
                        continue;

                    const auto hs = river_lut.get(c0, c1, b0, b1, b2, b3, b4);
                    const auto bin = std::min(std::size_t(hs * p.size()), p.size() - 1);
                    ++data_points[i][bin];
                }
            }

            const auto sum = std::accumulate(p.begin(), p.end(), 0);

            for (auto it = p.begin(); it != p.end(); ++it)
                *it /= sum;
        }

        std::vector<bucket_idx_t> buckets;
        std::vector<point_t> centers;

        k_means<point_t, bucket_idx_t, get_emd_distance, get_emd_cost>()(data_points, cluster_count,
            kmeans_max_iterations, OPTIMAL, &buckets, &centers);

        return buckets;
    }

    template<int Dimensions>
    std::vector<bucket_idx_t> create_turn_buckets(const hand_indexer& indexer, const holdem_river_lut& river_lut,
        int cluster_count, int kmeans_max_iterations)
    {
        typedef std::array<float, Dimensions> point_t;
        std::vector<point_t> data_points(indexer.get_size(indexer.get_rounds() - 1));

#pragma omp parallel for
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(data_points.size()); ++i)
        {
            auto& p = data_points[i];
            std::fill(p.begin(), p.end(), point_t::value_type());

            std::array<uint8_t, 6> cards;
            indexer.hand_unindex(indexer.get_rounds() - 1, i, cards.data());

            const auto c0 = cards[0];
            const auto c1 = cards[1];
            const auto b0 = cards[2];
            const auto b1 = cards[3];
            const auto b2 = cards[4];
            const auto b3 = cards[5];

            for (int b4 = 0; b4 < 52; ++b4)
            {
                if (b4 == c0 || b4 == c1 || b4 == b0 || b4 == b1 || b4 == b2 || b4 == b3)
                    continue;

                const auto hs = river_lut.get(c0, c1, b0, b1, b2, b3, b4);
                const auto bin = std::min(std::size_t(hs * p.size()), p.size() - 1);
                ++data_points[i][bin];
            }

            const auto sum = std::accumulate(p.begin(), p.end(), 0);

            for (auto it = p.begin(); it != p.end(); ++it)
                *it /= sum;
        }

        std::vector<bucket_idx_t> buckets;
        std::vector<point_t> centers;

        k_means<point_t, bucket_idx_t, get_emd_distance, get_emd_cost>()(data_points, cluster_count,
            kmeans_max_iterations, OPTIMAL, &buckets, &centers);

        return buckets;
    }

    template<int Dimensions>
    std::vector<bucket_idx_t> create_river_buckets(const holdem_river_ochs_lut& river_lut, int cluster_count,
        int kmeans_max_iterations)
    {
        typedef std::array<float, Dimensions> point_t;

        std::vector<bucket_idx_t> buckets;
        std::vector<point_t> centers;

        k_means<point_t, bucket_idx_t, get_l2_distance, get_l2_cost>()(river_lut.get_data(), cluster_count,
            kmeans_max_iterations, OPTIMAL, &buckets, &centers);

        return buckets;
    }

    std::vector<bucket_idx_t> create_buckets(const int round, const hand_indexer& indexer, const holdem_river_lut& river_lut,
        const holdem_river_ochs_lut& river_ochs_lut, int cluster_count, int kmeans_max_iterations)
    {
        const auto index_count = indexer.get_size(indexer.get_rounds() - 1);

        assert(cluster_count <= index_count);

        if (cluster_count == index_count)
        {
            std::vector<bucket_idx_t> buckets(cluster_count);
            std::iota(buckets.begin(), buckets.end(), 0);
            return buckets;
        }
        else
        {
            switch (round)
            {
            case holdem_abstraction_v2::PREFLOP:
                return create_preflop_buckets<50>(indexer, river_lut, cluster_count, kmeans_max_iterations);
            case holdem_abstraction_v2::FLOP:
                return create_flop_buckets<50>(indexer, river_lut, cluster_count, kmeans_max_iterations);
            case holdem_abstraction_v2::TURN:
                return create_turn_buckets<50>(indexer, river_lut, cluster_count, kmeans_max_iterations);
            case holdem_abstraction_v2::RIVER:
                return create_river_buckets<8>(river_ochs_lut, cluster_count, kmeans_max_iterations);
            }
        }

        return std::vector<bucket_idx_t>();
    }
}

holdem_abstraction_v2::holdem_abstraction_v2(bool imperfect_recall, const bucket_counts_t& bucket_counts,
    int kmeans_max_iterations)
    : imperfect_recall_(imperfect_recall)
{
    init();

    std::unique_ptr<holdem_river_lut> river_lut(new holdem_river_lut(std::ifstream("holdem_river_lut.dat",
        std::ios::binary)));
    std::shared_ptr<holdem_river_ochs_lut> river_ochs_lut(new holdem_river_ochs_lut(
        std::ifstream("holdem_river_ochs_lut.dat", std::ios::binary)));

    preflop_buckets_ = create_buckets(PREFLOP, *preflop_indexer_, *river_lut, *river_ochs_lut,
        bucket_counts[PREFLOP], kmeans_max_iterations);
    flop_buckets_ = create_buckets(FLOP, *flop_indexer_, *river_lut, *river_ochs_lut, bucket_counts[FLOP],
        kmeans_max_iterations);
    turn_buckets_ = create_buckets(TURN, *turn_indexer_, *river_lut, *river_ochs_lut, bucket_counts[TURN],
        kmeans_max_iterations);
    river_buckets_ = create_buckets(RIVER, *river_indexer_, *river_lut, *river_ochs_lut, bucket_counts[RIVER],
        kmeans_max_iterations);
}

holdem_abstraction_v2::holdem_abstraction_v2(const std::string& filename)
{
    std::ifstream is(filename, std::ios::binary);

    if (!is)
        throw std::runtime_error("bad istream");

    init();
    load(is);

    if (!is)
        throw std::runtime_error("read failed");
}

std::size_t holdem_abstraction_v2::get_bucket_count(const int round) const
{
    switch (round)
    {
    case PREFLOP: return preflop_buckets_.size();
    case FLOP: return flop_buckets_.size();
    case TURN: return turn_buckets_.size();
    case RIVER: return river_buckets_.size();
    default: return 0;
    }
}

void holdem_abstraction_v2::get_buckets(const int c0, const int c1, const int b0, const int b1, const int b2,
    const int b3, const int b4, bucket_type* buckets) const
{
    assert(c0 != -1 && c1 != -1);

    std::array<card_t, 7> cards = { card_t(c0), card_t(c1), card_t(b0), card_t(b1), card_t(b2), card_t(b3), card_t(b4) };

    (*buckets)[PREFLOP] = preflop_buckets_[preflop_indexer_->hand_index_last(cards.data())];

    if (b0 == -1)
        return;

    (*buckets)[FLOP] = flop_buckets_[flop_indexer_->hand_index_last(cards.data())];

    if (b3 == -1)
        return;

    (*buckets)[TURN] = turn_buckets_[turn_indexer_->hand_index_last(cards.data())];

    if (b4 == -1)
        return;

    (*buckets)[RIVER] = river_buckets_[river_indexer_->hand_index_last(cards.data())];
}

int holdem_abstraction_v2::get_bucket(int c0, int c1) const
{
    bucket_type buckets;
    get_buckets(c0, c1, -1, -1, -1, -1, -1, &buckets);
    return buckets[PREFLOP];
}

int holdem_abstraction_v2::get_bucket(int c0, int c1, int b0, int b1, int b2) const
{
    bucket_type buckets;
    get_buckets(c0, c1, b0, b1, b2, -1, -1, &buckets);
    return buckets[FLOP];
}

int holdem_abstraction_v2::get_bucket(int c0, int c1, int b0, int b1, int b2, int b3) const
{
    bucket_type buckets;
    get_buckets(c0, c1, b0, b1, b2, b3, -1, &buckets);
    return buckets[TURN];
}

int holdem_abstraction_v2::get_bucket(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const
{
    bucket_type buckets;
    get_buckets(c0, c1, b0, b1, b2, b3, b4, &buckets);
    return buckets[RIVER];
}

void holdem_abstraction_v2::save(std::ostream& os) const
{
    binary_write(os, imperfect_recall_);
    binary_write(os, preflop_buckets_);
    binary_write(os, flop_buckets_);
    binary_write(os, turn_buckets_);
    binary_write(os, river_buckets_);
}

void holdem_abstraction_v2::load(std::istream& is)
{
    binary_read(is, imperfect_recall_);
    binary_read(is, preflop_buckets_);
    binary_read(is, flop_buckets_);
    binary_read(is, turn_buckets_);
    binary_read(is, river_buckets_);
}

void holdem_abstraction_v2::init()
{
    std::vector<std::uint8_t> cfg = boost::assign::list_of(2);
    preflop_indexer_.reset(new hand_indexer(cfg));

    cfg = boost::assign::list_of(2)(3);
    flop_indexer_.reset(new hand_indexer(cfg));

    cfg = boost::assign::list_of(2)(4);
    turn_indexer_.reset(new hand_indexer(cfg));

    cfg = boost::assign::list_of(2)(5);
    river_indexer_.reset(new hand_indexer(cfg));
}
