#include "holdem_river_ochs_lut.h"
#include <omp.h>
#include <iostream>
#include <numeric>
#include <boost/format.hpp>
#include <boost/assign.hpp>
#include "evallib/holdem_evaluator.h"
#include "util/sort.h"
#include "util/card.h"
#include "util/holdem_loops.h"

namespace
{
    std::unique_ptr<hand_indexer> create()
    {
        std::vector<card_t> cfg;
        cfg.push_back(2);
        cfg.push_back(5);
        return std::unique_ptr<hand_indexer>(new hand_indexer(cfg));
    }
}

holdem_river_ochs_lut::holdem_river_ochs_lut()
    : indexer_(create())
{
    data_.resize(indexer_->get_size(indexer_->get_rounds() - 1));

    std::cout << "create_preflop_buckets start\n";

    std::array<int, 169> preflop_clusters =
    {
        4, 1, 6, 1, 1, 6, 1, 1, 1, 6, 1, 1, 1,
        1, 7, 1, 1, 1, 2, 3, 7, 1, 1, 2, 2, 3,
        3, 8, 2, 2, 2, 2, 3, 3, 3, 8, 2, 2, 2,
        2, 3, 3, 3, 5, 8, 2, 2, 4, 4, 4, 4, 5,
        5, 5, 8, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
        8, 4, 4, 4, 6, 6, 6, 6, 6, 7, 7, 7, 8,
        6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 8,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 2,
        3, 3, 2, 2, 2, 3, 3, 3, 2, 2, 2, 3, 3,
        3, 3, 2, 3, 3, 3, 3, 5, 5, 5, 4, 4, 4,
        4, 4, 5, 5, 5, 5, 4, 4, 4, 4, 5, 5, 5,
        5, 7, 7, 4, 6, 6, 6, 6, 6, 6, 7, 7, 7,
        7, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7
    };

    std::vector<card_t> cfg;
    cfg.push_back(2);
    std::unique_ptr<hand_indexer> preflop_indexer(new hand_indexer(cfg));

    holdem_evaluator eval;
    int foo = 0;

#pragma omp parallel for
    for (std::int64_t i = 0; i < static_cast<std::int64_t>(data_.size()); ++i)
    {
        auto& p = data_[i];

        std::array<uint8_t, 7> cards;
        indexer_->hand_unindex(indexer_->get_rounds() - 1, i, cards.data());

        data_type sum = {};
        std::array<uint8_t, 2> opp_cards;

        for (uint8_t c0 = 0; c0 < 51; ++c0)
        {
            if (c0 == cards[0] || c0 == cards[1] || c0 == cards[2] || c0 == cards[3] || c0 == cards[4] || c0 == cards[5] || c0 == cards[6])
                continue;

            opp_cards[0] = c0;

            for (uint8_t c1 = c0 + 1; c1 < 52; ++c1)
            {
                if (c1 == cards[0] || c1 == cards[1] || c1 == cards[2] || c1 == cards[3] || c1 == cards[4] || c1 == cards[5] || c1 == cards[6])
                    continue;

                opp_cards[1] = c1;

                const auto v0 = eval.get_hand_value(cards[0], cards[1], cards[2], cards[3], cards[4], cards[5], cards[6]);
                const auto v1 = eval.get_hand_value(c0, c1, cards[2], cards[3], cards[4], cards[5], cards[6]);

                const auto preflop_idx = preflop_indexer->hand_index_last(opp_cards.data());
                const auto preflop_cluster = preflop_clusters[preflop_idx] - 1; // subtract one as the cluster numbers are 1-based
                p[preflop_cluster] += (v0 > v1) ? 1 : ((v0 == v1) ? 0.5f : 0.0f);

                ++sum[preflop_cluster];
            }
        }

        for (std::size_t j = 0; j < p.size(); ++j)
            p[j] /= sum[j];

        if (foo % 10000 == 0 && omp_get_thread_num() == 0)
        {
            std::cout << foo << "\t" << get_card_string(cards[0]) << get_card_string(cards[1]) << " | " << get_card_string(cards[2]) << get_card_string(cards[3]) << get_card_string(cards[4]) << get_card_string(cards[5]) << get_card_string(cards[6])
                << "\t" << p[0] << ", " << p[1] << ", " << p[2] << ", " << p[3] << ", " << p[4] << ", " << p[5] << ", " << p[6] << ", " << p[7] << "\n";
        }
#pragma omp atomic
        ++foo;
    }
}

holdem_river_ochs_lut::holdem_river_ochs_lut(std::istream&& is)
    : indexer_(create())
{
    data_.resize(indexer_->get_size(indexer_->get_rounds() - 1));

    if (!is)
        throw std::runtime_error("bad istream");

    is.read(reinterpret_cast<char*>(&data_[0]), sizeof(data_type) * data_.size());

    if (!is)
        throw std::runtime_error("read failed");
}

void holdem_river_ochs_lut::save(std::ostream& os) const
{
    os.write(reinterpret_cast<const char*>(&data_[0]), sizeof(data_type) * data_.size());
}

const holdem_river_ochs_lut::data_type& holdem_river_ochs_lut::get_data(const std::array<card_t, 7>& cards) const
{
    return data_[get_key(cards)];
}

hand_indexer::hand_index_t holdem_river_ochs_lut::get_key(const std::array<card_t, 7>& cards) const
{
    return indexer_->hand_index_last(cards.data());
}

const std::vector<holdem_river_ochs_lut::data_type>& holdem_river_ochs_lut::get_data() const
{
    return data_;
}
