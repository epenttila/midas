#include "holdem_preflop_lut.h"
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <bitset>
#include <omp.h>
#include <iostream>
#include <boost/format.hpp>
#include <boost/log/trivial.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "evallib/holdem_evaluator.h"
#include "util/card.h"
#include "util/binary_io.h"

holdem_preflop_lut::holdem_preflop_lut()
{
    std::vector<bool> generated(data_.size());
    holdem_evaluator e;

    const double start_time = omp_get_wtime();
    double time = start_time;
    int iteration = 0;
    int keys = 0;

    for (int a = 0; a < 51; ++a)
    {
#pragma omp parallel for
        for (int b = a + 1; b < 52; ++b)
        {
            const auto key = get_key(a, b);

            if (!generated[key])
            {
                generated[key] = true;
                const result r = e.enumerate_preflop(a, b);
                data_[key] = std::make_pair(float(r.ehs), float(r.ehs2));
#pragma omp atomic
                ++keys;
            }

#pragma omp atomic
            ++iteration;

            const double t = omp_get_wtime();

            if (iteration == 1326 || (omp_get_thread_num() == 0 && t - time >= 1))
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
        }
    }
}

holdem_preflop_lut::holdem_preflop_lut(const std::string& filename)
{
    BOOST_LOG_TRIVIAL(info) << "Loading preflop lut from: " << filename;

    auto file = binary_open(filename, "rb");

    if (!file)
        throw std::runtime_error("bad file");

    binary_read(*file, data_.data(), data_.size());
}

void holdem_preflop_lut::save(const std::string& filename) const
{
    auto file = binary_open(filename, "wb");

    binary_write(*file, data_.data(), data_.size());
}

const holdem_preflop_lut::data_type& holdem_preflop_lut::get(int c0, int c1) const
{
    return data_[get_key(c0, c1)];
}

int holdem_preflop_lut::get_key(int c0, int c1) const
{
    assert(c0 != c1);

    const std::pair<int, int> ranks = std::minmax(get_rank(c0), get_rank(c1));
    const int suits[2] = {get_suit(c0), get_suit(c1)};

    if (suits[0] == suits[1])
        return ranks.first * 13 + ranks.second;
    else
        return ranks.second * 13 + ranks.first;
}
