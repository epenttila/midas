#include "holdem_abstraction.h"
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <omp.h>
#include <numeric>
#include <boost/regex.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "util/k_means.h"
#include "util/binary_io.h"
#include "lutlib/hand_indexer.h"
#include "util/metric.h"
#include "lutlib/holdem_river_lut.h"
#include "lutlib/holdem_river_ochs_lut.h"

namespace
{
    typedef holdem_abstraction::bucket_idx_t bucket_idx_t;

    template<int Dimensions>
    std::vector<bucket_idx_t> create_preflop_buckets(const hand_indexer& indexer, const holdem_river_lut& river_lut,
        int cluster_count, int kmeans_max_iterations, float tolerance, int runs)
    {
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

                                const std::array<int, 7> int_cards = {{c0, c1, b0, b1, b2, b3, b4}};
                                const auto hs = river_lut.get(int_cards);
                                const auto bin = std::min(std::size_t(hs * p.size()), p.size() - 1);
                                ++data_points[i][bin];
                            }
                        }
                    }
                }
            }

            const auto sum = std::accumulate(p.begin(), p.end(), typename point_t::value_type());

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

                    const std::array<int, 7> int_cards = {{c0, c1, b0, b1, b2, b3, b4}};
                    const auto hs = river_lut.get(int_cards);
                    const auto bin = std::min(std::size_t(hs * p.size()), p.size() - 1);
                    ++data_points[i][bin];
                }
            }

            const auto sum = std::accumulate(p.begin(), p.end(), typename point_t::value_type());

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

                const std::array<int, 7> int_cards = {{c0, c1, b0, b1, b2, b3, b4}};
                const auto hs = river_lut.get(int_cards);
                const auto bin = std::min(std::size_t(hs * p.size()), p.size() - 1);
                ++data_points[i][bin];
            }

            const auto sum = std::accumulate(p.begin(), p.end(), typename point_t::value_type());

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
        typedef std::array<float, Dimensions> point_t;

        std::vector<bucket_idx_t> buckets;
        std::vector<point_t> centers;

        k_means<point_t, bucket_idx_t, get_l2_distance, get_l2_cost>().run(river_lut.get_data(), cluster_count,
            kmeans_max_iterations, tolerance, OPTIMAL, runs, &buckets, &centers);

        return buckets;
    }

    std::vector<bucket_idx_t> create_buckets(const holdem_state::game_round round, const hand_indexer& indexer,
        const holdem_river_lut& river_lut, const holdem_river_ochs_lut& river_ochs_lut, int cluster_count,
        int kmeans_max_iterations, float tolerance, int runs)
    {
        const auto index_count = static_cast<bucket_idx_t>(indexer.get_size(indexer.get_rounds() - 1));

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
            case holdem_state::PREFLOP:
                return create_preflop_buckets<50>(indexer, river_lut, cluster_count, kmeans_max_iterations, tolerance,
                    runs);
            case holdem_state::FLOP:
                return create_flop_buckets<50>(indexer, river_lut, cluster_count, kmeans_max_iterations, tolerance,
                    runs);
            case holdem_state::TURN:
                return create_turn_buckets<50>(indexer, river_lut, cluster_count, kmeans_max_iterations, tolerance,
                    runs);
            case holdem_state::RIVER:
                return create_river_buckets<8>(river_ochs_lut, cluster_count, kmeans_max_iterations, tolerance,
                    runs);
            default:
                throw std::runtime_error("invalid round");
            }
        }
    }

    void parse_configuration(const std::string& filename, bool* imperfect_recall,
        holdem_abstraction::bucket_counts_t* counts)
    {
        const auto configuration = boost::filesystem::path(filename).stem().string();

        boost::regex r("(pr|ir)-(\\d+)-(\\d+)-(\\d+)-(\\d+).*");
        boost::smatch m;

        if (!boost::regex_match(configuration, m, r))
            throw std::runtime_error("Invalid abstraction configuration");

        *imperfect_recall = m[1] == "ir" ? true : false;
        (*counts)[holdem_state::PREFLOP] = std::stoi(m[2]);
        (*counts)[holdem_state::FLOP] = std::stoi(m[3]);
        (*counts)[holdem_state::TURN] = std::stoi(m[4]);
        (*counts)[holdem_state::RIVER] = std::stoi(m[5]);
    }

    std::vector<std::uint8_t> make_configuration(std::uint8_t preflop, std::uint8_t postflop)
    {
        if (postflop == 0)
            return boost::assign::list_of(preflop);
        else
            return boost::assign::list_of(preflop)(postflop);
    }
}

const hand_indexer holdem_abstraction::preflop_indexer_ = hand_indexer(make_configuration(2, 0));
const hand_indexer holdem_abstraction::flop_indexer_ = hand_indexer(make_configuration(2, 3));
const hand_indexer holdem_abstraction::turn_indexer_ = hand_indexer(make_configuration(2, 4));
const hand_indexer holdem_abstraction::river_indexer_ = hand_indexer(make_configuration(2, 5));

holdem_abstraction::holdem_abstraction()
    : imperfect_recall_(false)
    , bucket_counts_()
    , file_()
{
}

int holdem_abstraction::get_bucket_count(const holdem_state::game_round round) const
{
    return bucket_counts_[round];
}

void holdem_abstraction::get_buckets(const int c0, const int c1, const int b0, const int b1, const int b2,
    const int b3, const int b4, bucket_type* buckets) const
{
    assert(c0 != -1 && c1 != -1);

    const std::array<card_t, 7> cards = {{
        static_cast<card_t>(c0),
        static_cast<card_t>(c1),
        static_cast<card_t>(b0),
        static_cast<card_t>(b1),
        static_cast<card_t>(b2),
        static_cast<card_t>(b3),
        static_cast<card_t>(b4)}};

    (*buckets)[holdem_state::PREFLOP] = read(holdem_state::PREFLOP, preflop_indexer_.hand_index_last(cards.data()));
    (*buckets)[holdem_state::FLOP] = (b0 != -1) ? read(holdem_state::FLOP, flop_indexer_.hand_index_last(cards.data())) : -1;
    (*buckets)[holdem_state::TURN] = (b3 != -1) ? read(holdem_state::TURN, turn_indexer_.hand_index_last(cards.data())) : -1;
    (*buckets)[holdem_state::RIVER] = (b4 != -1) ? read(holdem_state::RIVER, river_indexer_.hand_index_last(cards.data())) : -1;
}

void holdem_abstraction::read(const std::string& filename)
{
    parse_configuration(filename, &imperfect_recall_, &bucket_counts_);

    file_.open(filename);

    if (!file_)
        throw std::runtime_error("Unable to open file");
}

void holdem_abstraction::write(const std::string& filename, const int kmeans_max_iterations, float tolerance,
    int runs)
{
    parse_configuration(filename, &imperfect_recall_, &bucket_counts_);

    std::unique_ptr<holdem_river_lut> river_lut(new holdem_river_lut("holdem_river_lut.dat"));
    std::shared_ptr<holdem_river_ochs_lut> river_ochs_lut(new holdem_river_ochs_lut("holdem_river_ochs_lut.dat"));

    const auto preflop_buckets = create_buckets(holdem_state::PREFLOP, preflop_indexer_, *river_lut, *river_ochs_lut,
        bucket_counts_[holdem_state::PREFLOP], kmeans_max_iterations, tolerance, runs);
    const auto flop_buckets = create_buckets(holdem_state::FLOP, flop_indexer_, *river_lut, *river_ochs_lut,
        bucket_counts_[holdem_state::FLOP], kmeans_max_iterations, tolerance, runs);
    const auto turn_buckets = create_buckets(holdem_state::TURN, turn_indexer_, *river_lut, *river_ochs_lut,
        bucket_counts_[holdem_state::TURN], kmeans_max_iterations, tolerance, runs);
    const auto river_buckets = create_buckets(holdem_state::RIVER, river_indexer_, *river_lut, *river_ochs_lut,
        bucket_counts_[holdem_state::RIVER], kmeans_max_iterations, tolerance, runs);

    auto file = binary_open(filename.c_str(), "wb");

    if (!file)
        throw std::runtime_error("Unable to open file");

    binary_write(*file, imperfect_recall_);
    binary_write(*file, preflop_buckets);
    binary_write(*file, flop_buckets);
    binary_write(*file, turn_buckets);
    binary_write(*file, river_buckets);
}

holdem_abstraction::bucket_idx_t holdem_abstraction::read(holdem_state::game_round round,
    hand_indexer::hand_index_t index) const
{
    if (round < 0 || round > holdem_state::RIVER)
    {
        assert(false);
        return 0;
    }

    std::size_t pos = sizeof(imperfect_recall_);

    switch (round)
    {
    case holdem_state::RIVER:
        pos += sizeof(std::uint64_t) + turn_indexer_.get_size(turn_indexer_.get_rounds() - 1) * sizeof(bucket_idx_t);
    case holdem_state::TURN:
        pos += sizeof(std::uint64_t) + flop_indexer_.get_size(flop_indexer_.get_rounds() - 1) * sizeof(bucket_idx_t);
    case holdem_state::FLOP:
        pos += sizeof(std::uint64_t) + preflop_indexer_.get_size(preflop_indexer_.get_rounds() - 1)
            * sizeof(bucket_idx_t);
    case holdem_state::PREFLOP:
        pos += sizeof(std::uint64_t);
        break;
    default:
        throw std::runtime_error("invalid round");
    }

    pos += index * sizeof(bucket_idx_t);

    return *reinterpret_cast<const bucket_idx_t*>(file_.data() + pos);
}
