#include "holdem_river_lut.h"
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <omp.h>
#include <iostream>
#include <boost/format.hpp>
#include <boost/log/trivial.hpp>
#include <cstring>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "evallib/holdem_evaluator.h"
#include "util/sort.h"
#include "util/card.h"
#include "util/holdem_loops.h"
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

    holdem_evaluator e;
    std::vector<bool> generated(data_.size());

    const double start_time = omp_get_wtime();
    double time = start_time;
    unsigned int iteration = 0;
    int keys = 0;

#pragma omp parallel
    {
        parallel_for_each_river([&](int a, int b, int i, int j, int k, int l, int m) {
            const std::array<int, 7> cards = {{a, b, i, j, k, l, m}};
            const auto key = get_key(cards);

            if (!generated[key])
            {
                generated[key] = true;
                data_[key] = holdem_river_lut::data_type(e.enumerate_river(a, b, i, j, k, l, m));
#pragma omp atomic
                ++keys;
            }

#pragma omp atomic
            ++iteration;

            const double t = omp_get_wtime();

            if (iteration == 2809475760 || (omp_get_thread_num() == 0 && t - time >= 1))
            {
                // TODO use same progress system as cfr
                const double duration = t - start_time;
                const int hour = int(duration / 3600);
                const int minute = int(duration / 60 - hour * 60);
                const int second = int(duration - minute * 60 - hour * 3600);
                const int ips = int(iteration / duration);
                std::cout << boost::format("%02d:%02d:%02d: %d/%d (%d i/s)\n") %
                    hour % minute % second % keys % iteration % ips;
                time = t;
            }
        });
    }
}

holdem_river_lut::holdem_river_lut(const std::string& filename)
{
    BOOST_LOG_TRIVIAL(info) << "Loading river lut from: " << filename;

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
