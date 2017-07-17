#include "holdem_river_lut.h"
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <omp.h>
#include <iostream>
#include <boost/format.hpp>
#include <cstring>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "evallib/holdem_evaluator.h"
#include "util/sort.h"
#include "util/card.h"
#include "util/binary_io.h"

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

holdem_river_lut::holdem_river_lut()
    : indexer_(create())
{
    data_.resize(indexer_->get_size(indexer_->get_rounds() - 1));

    std::unique_ptr<holdem_evaluator> e(new holdem_evaluator);

    const double start_time = omp_get_wtime();
    double time = start_time;
    unsigned int iteration = 0;

#pragma omp parallel for
    for (std::int64_t i = 0; i < static_cast<std::int64_t>(data_.size()); ++i)
    {
        std::array<card_t, 7> cards;
        indexer_->hand_unindex(indexer_->get_rounds() - 1, i, cards.data());

        data_[i] = holdem_river_lut::data_type(e->enumerate_river(cards[0], cards[1], cards[2], cards[3], cards[4], cards[5], cards[6]));

#pragma omp atomic
        ++iteration;

        const double t = omp_get_wtime();

        if (iteration == data_.size() || (omp_get_thread_num() == 0 && t - time >= 1))
        {
            // TODO use same progress system as cfr
            const double duration = t - start_time;
            const int hour = int(duration / 3600);
            const int minute = int(duration / 60 - hour * 60);
            const int second = int(duration - minute * 60 - hour * 3600);
            const int ips = int(iteration / duration);
            std::cout << boost::format("%02d:%02d:%02d: %d/%d (%d i/s)\n") %
                hour % minute % second % iteration % data_.size() % ips;
            time = t;
        }
    }
}

holdem_river_lut::holdem_river_lut(const std::string& filename)
    : indexer_(create())
{
    data_.resize(indexer_->get_size(indexer_->get_rounds() - 1));

    auto file = binary_open(filename, "rb");

    if (!file)
        throw std::runtime_error("bad file");

    binary_read(*file, data_.data(), data_.size());
}

void holdem_river_lut::save(const std::string& filename) const
{
    auto file = binary_open(filename, "wb");

    binary_write(*file, data_.data(), data_.size());
}

const holdem_river_lut::data_type& holdem_river_lut::get(const std::array<int, 7>& cards) const
{
    return data_[get_key(cards)];
}

hand_indexer::hand_index_t holdem_river_lut::get_key(const std::array<int, 7>& cards) const
{
    const std::array<card_t, 7> c = {{
        static_cast<card_t>(cards[0]),
        static_cast<card_t>(cards[1]),
        static_cast<card_t>(cards[2]),
        static_cast<card_t>(cards[3]),
        static_cast<card_t>(cards[4]),
        static_cast<card_t>(cards[5]),
        static_cast<card_t>(cards[6])}};

    return indexer_->hand_index_last(c.data());
}
