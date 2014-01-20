#include "holdem_abstraction_v2.h"
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <fstream>
#include <omp.h>
#include <numeric>
#include <boost/regex.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "util/k_means.h"
#include "util/binary_io.h"
#include "lutlib/hand_indexer.h"
#include "util/metric.h"

namespace
{
    typedef holdem_abstraction_v2::bucket_idx_t bucket_idx_t;

    template<int Dimensions>
    std::vector<bucket_idx_t> create_preflop_buckets(const hand_indexer& indexer, const holdem_river_lut& river_lut,
        int cluster_count, int kmeans_max_iterations, float tolerance, int runs)
    {
        BOOST_LOG_TRIVIAL(info) << "Creating preflop buckets";

        typedef std::array<float, Dimensions> point_t;
        std::vector<point_t> data_points(indexer.get_size(indexer.get_rounds() - 1));

#pragma omp parallel for
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(data_points.size()); ++i)
        {
            auto& p = data_points[i];
            std::fill(p.begin(), p.end(), typename point_t::value_type());

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

        k_means<point_t, bucket_idx_t, get_emd_distance, get_emd_cost>().run(data_points, cluster_count,
            kmeans_max_iterations, tolerance, OPTIMAL, runs, &buckets, &centers);

        return buckets;
    }

    template<int Dimensions>
    std::vector<bucket_idx_t> create_flop_buckets(const hand_indexer& indexer, const holdem_river_lut& river_lut,
        int cluster_count, int kmeans_max_iterations, float tolerance, int runs)
    {
        BOOST_LOG_TRIVIAL(info) << "Creating flop buckets";

        typedef std::array<float, Dimensions> point_t;
        std::vector<point_t> data_points(indexer.get_size(indexer.get_rounds() - 1));

#pragma omp parallel for
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(data_points.size()); ++i)
        {
            auto& p = data_points[i];
            std::fill(p.begin(), p.end(), typename point_t::value_type());

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

        k_means<point_t, bucket_idx_t, get_emd_distance, get_emd_cost>().run(data_points, cluster_count,
            kmeans_max_iterations, tolerance, OPTIMAL, runs, &buckets, &centers);

        return buckets;
    }

    template<int Dimensions>
    std::vector<bucket_idx_t> create_turn_buckets(const hand_indexer& indexer, const holdem_river_lut& river_lut,
        int cluster_count, int kmeans_max_iterations, float tolerance, int runs)
    {
        BOOST_LOG_TRIVIAL(info) << "Creating turn buckets";

        typedef std::array<float, Dimensions> point_t;
        std::vector<point_t> data_points(indexer.get_size(indexer.get_rounds() - 1));

#pragma omp parallel for
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(data_points.size()); ++i)
        {
            auto& p = data_points[i];
            std::fill(p.begin(), p.end(), typename point_t::value_type());

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

        k_means<point_t, bucket_idx_t, get_emd_distance, get_emd_cost>().run(data_points, cluster_count,
            kmeans_max_iterations, tolerance, OPTIMAL, runs, &buckets, &centers);

        return buckets;
    }

    template<int Dimensions>
    std::vector<bucket_idx_t> create_river_buckets(const holdem_river_ochs_lut& river_lut, int cluster_count,
        int kmeans_max_iterations, float tolerance, int runs)
    {
        BOOST_LOG_TRIVIAL(info) << "Creating river buckets";

        typedef std::array<float, Dimensions> point_t;

        std::vector<bucket_idx_t> buckets;
        std::vector<point_t> centers;

        k_means<point_t, bucket_idx_t, get_l2_distance, get_l2_cost>().run(river_lut.get_data(), cluster_count,
            kmeans_max_iterations, tolerance, OPTIMAL, runs, &buckets, &centers);

        return buckets;
    }

    std::vector<bucket_idx_t> create_buckets(const int round, const hand_indexer& indexer, const holdem_river_lut& river_lut,
        const holdem_river_ochs_lut& river_ochs_lut, std::size_t cluster_count, std::size_t kmeans_max_iterations,
        float tolerance, int runs)
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
            case PREFLOP:
                return create_preflop_buckets<50>(indexer, river_lut, cluster_count, kmeans_max_iterations, tolerance,
                    runs);
            case FLOP:
                return create_flop_buckets<50>(indexer, river_lut, cluster_count, kmeans_max_iterations, tolerance,
                    runs);
            case TURN:
                return create_turn_buckets<50>(indexer, river_lut, cluster_count, kmeans_max_iterations, tolerance,
                    runs);
            case RIVER:
                return create_river_buckets<8>(river_ochs_lut, cluster_count, kmeans_max_iterations, tolerance,
                    runs);
            }
        }

        return std::vector<bucket_idx_t>();
    }

    void parse_configuration(const std::string& filename, bool* imperfect_recall,
        holdem_abstraction_v2::bucket_counts_t* counts)
    {
        const auto configuration = boost::filesystem::path(filename).stem().string();

        boost::regex r("(pr|ir)-(\\d+)-(\\d+)-(\\d+)-(\\d+).*");
        boost::smatch m;

        if (!boost::regex_match(configuration, m, r))
            throw std::runtime_error("Invalid abstraction configuration");

        *imperfect_recall = m[1] == "ir" ? true : false;
        (*counts)[PREFLOP] = std::stoi(m[2]);
        (*counts)[FLOP] = std::stoi(m[3]);
        (*counts)[TURN] = std::stoi(m[4]);
        (*counts)[RIVER] = std::stoi(m[5]);
    }

    std::vector<std::uint8_t> make_configuration(std::uint8_t preflop, std::uint8_t postflop)
    {
        if (postflop == 0)
            return boost::assign::list_of(preflop);
        else
            return boost::assign::list_of(preflop)(postflop);
    }
}

const hand_indexer holdem_abstraction_v2::preflop_indexer_ = hand_indexer(make_configuration(2, 0));
const hand_indexer holdem_abstraction_v2::flop_indexer_ = hand_indexer(make_configuration(2, 3));
const hand_indexer holdem_abstraction_v2::turn_indexer_ = hand_indexer(make_configuration(2, 4));
const hand_indexer holdem_abstraction_v2::river_indexer_ = hand_indexer(make_configuration(2, 5));

holdem_abstraction_v2::holdem_abstraction_v2()
    : imperfect_recall_(false)
    , bucket_counts_()
    , file_()
{
}

int holdem_abstraction_v2::get_bucket_count(const int round) const
{
    return bucket_counts_[round];
}

void holdem_abstraction_v2::get_buckets(const int c0, const int c1, const int b0, const int b1, const int b2,
    const int b3, const int b4, bucket_type* buckets) const
{
    assert(c0 != -1 && c1 != -1);

    std::array<card_t, 7> cards = {{card_t(c0), card_t(c1), card_t(b0), card_t(b1), card_t(b2), card_t(b3), card_t(b4)}};

    (*buckets)[PREFLOP] = read(PREFLOP, preflop_indexer_.hand_index_last(cards.data()));
    (*buckets)[FLOP] = (b0 != -1) ? read(FLOP, flop_indexer_.hand_index_last(cards.data())) : -1;
    (*buckets)[TURN] = (b3 != -1) ? read(TURN, turn_indexer_.hand_index_last(cards.data())) : -1;
    (*buckets)[RIVER] = (b4 != -1) ? read(RIVER, river_indexer_.hand_index_last(cards.data())) : -1;
}

void holdem_abstraction_v2::read(const std::string& filename)
{
    BOOST_LOG_TRIVIAL(info) << "Reading abstraction: " << filename;

    parse_configuration(filename, &imperfect_recall_, &bucket_counts_);

    file_.open(filename);

    if (!file_)
        throw std::runtime_error("Unable to open file");
}

void holdem_abstraction_v2::write(const std::string& filename, const int kmeans_max_iterations, float tolerance,
    int runs)
{
    BOOST_LOG_TRIVIAL(info) << "Generating abstraction: " << filename;

    parse_configuration(filename, &imperfect_recall_, &bucket_counts_);

    std::unique_ptr<holdem_river_lut> river_lut(new holdem_river_lut(std::ifstream("holdem_river_lut.dat",
        std::ios::binary)));
    std::shared_ptr<holdem_river_ochs_lut> river_ochs_lut(new holdem_river_ochs_lut(
        std::ifstream("holdem_river_ochs_lut.dat", std::ios::binary)));

    const auto preflop_buckets = create_buckets(PREFLOP, preflop_indexer_, *river_lut, *river_ochs_lut,
        bucket_counts_[PREFLOP], kmeans_max_iterations, tolerance, runs);
    const auto flop_buckets = create_buckets(FLOP, flop_indexer_, *river_lut, *river_ochs_lut, bucket_counts_[FLOP],
        kmeans_max_iterations, tolerance, runs);
    const auto turn_buckets = create_buckets(TURN, turn_indexer_, *river_lut, *river_ochs_lut, bucket_counts_[TURN],
        kmeans_max_iterations, tolerance, runs);
    const auto river_buckets = create_buckets(RIVER, river_indexer_, *river_lut, *river_ochs_lut,
        bucket_counts_[RIVER], kmeans_max_iterations, tolerance, runs);

    BOOST_LOG_TRIVIAL(info) << "Saving abstraction: " << filename;

    std::ofstream os(filename, std::ios::binary);

    if (!os)
        throw std::runtime_error("Unable to open file");

    binary_write(os, imperfect_recall_);
    binary_write(os, preflop_buckets);
    binary_write(os, flop_buckets);
    binary_write(os, turn_buckets);
    binary_write(os, river_buckets);
}

holdem_abstraction_v2::bucket_idx_t holdem_abstraction_v2::read(int round, hand_indexer::hand_index_t index) const
{
    if (round < 0 || round > RIVER)
    {
        assert(false);
        return 0;
    }

    std::size_t pos = sizeof(imperfect_recall_);

    switch (round)
    {
    case RIVER:
        pos += sizeof(std::uint64_t) + turn_indexer_.get_size(turn_indexer_.get_rounds() - 1) * sizeof(bucket_idx_t);
    case TURN:
        pos += sizeof(std::uint64_t) + flop_indexer_.get_size(flop_indexer_.get_rounds() - 1) * sizeof(bucket_idx_t);
    case FLOP:
        pos += sizeof(std::uint64_t) + preflop_indexer_.get_size(preflop_indexer_.get_rounds() - 1)
            * sizeof(bucket_idx_t);
    case PREFLOP:
        pos += sizeof(std::uint64_t);
    }

    pos += index * sizeof(bucket_idx_t);

    return *reinterpret_cast<const bucket_idx_t*>(file_.data() + pos);
}
