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
    static const bool board_rank_suits[holdem_river_lut::RANK_COUNT][holdem_river_lut::SUIT_COUNT] =
    {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
        {0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1},
        {0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1},
        {0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    };

    static const int hole_suit_offsets[holdem_river_lut::SUIT_COUNT][4] =
    {
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
         {0,  1,  2,  3},
        {-1, -1, -1,  0},
    };
}

holdem_river_lut::holdem_river_lut()
{
    init();

    holdem_evaluator e;
    std::vector<bool> generated(data_.size());

    const double start_time = omp_get_wtime();
    double time = start_time;
    unsigned int iteration = 0;
    int keys = 0;

#pragma omp parallel
    {
        parallel_for_each_river([&](int a, int b, int i, int j, int k, int l, int m) {
            const auto key = get_key(a, b, i, j, k, l, m);

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

    init();

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

const holdem_river_lut::data_type& holdem_river_lut::get(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const
{
    return data_[get_key(c0, c1, b0, b1, b2, b3, b4)];
}

int holdem_river_lut::get_key(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3, const int b4) const
{
    assert(c0 != c1 && c0 != b0 && c0 != b1 && c0 != b2 && c0 != b3 && c0 != b4
        && c1 != b0 && c1 != b1 && c1 != b2 && c1 != b3 && c1 != b4
        && b0 != b1 && b0 != b2 && b0 != b3 && b0 != b4
        && b1 != b2 && b1 != b3 && b1 != b4
        && b2 != b3 && b2 != b4
        && b3 != b4);

    const int board_cards = 5;
    std::array<int, board_cards> board = {{b0, b1, b2, b3, b4}};

    sort(board);

    const std::array<int, board_cards> board_ranks = {{get_rank(board[0]), get_rank(board[1]), get_rank(board[2]), get_rank(board[3]), get_rank(board[4])}};
    const std::array<int, board_cards> board_suits = {{get_suit(board[0]), get_suit(board[1]), get_suit(board[2]), get_suit(board[3]), get_suit(board[4])}};

    // board rank pattern and offset
    const int board_rank = rank_indexes_[3][board_ranks[0]] + rank_indexes_[2][board_ranks[1]] + rank_indexes_[1][board_ranks[2]] + rank_indexes_[0][board_ranks[3]] + board_ranks[4];
    const int board_rank_pattern = board_rank_patterns_[board_rank].first;
    const int board_rank_offset = board_rank_patterns_[board_rank].second;

    // board suit pattern
    const int board_suit_pattern = board_suit_patterns_[board_suits[0] * 256 + board_suits[1] * 64 + board_suits[2] * 16 + board_suits[3] * 4 + board_suits[4]];

    // suit mapping base 2 (A=0, O=1)
    std::array<int, 4> suit_counts = {{}};

    for (std::size_t i = 0; i < board.size(); ++i)
        ++suit_counts[board_suits[i]];

    const int suit_base = 2;
    std::array<int, 4> suit_map = {{suit_base - 1, suit_base - 1, suit_base - 1, suit_base - 1}};

    for (std::size_t i = 0; i < suit_counts.size(); ++i)
    {
        if (suit_counts[i] >= 3)
        {
            suit_map[i] = 0; // only one possible suit makes a flush
            break;
        }
    }

    // board+hole suit combo counts
    const int board_rank_suit_offset = board_rank_suit_offsets_[board_rank_pattern][board_suit_pattern];
    assert(board_rank_suit_offset != -1);

    // hole
    std::array<int, 2> hole = {{c0, c1}};
    sort(hole);

    const std::array<int, 2> hole_ranks = {{get_rank(hole[0]), get_rank(hole[1])}};
    const std::array<int, 2> hole_iso_suits = {{suit_map[get_suit(hole[0])], suit_map[get_suit(hole[1])]}};

    const int hole_rank = rank_indexes_[0][hole_ranks[0]] + hole_ranks[1];
    const int hole_suit = hole_iso_suits[0] * suit_base + hole_iso_suits[1];

    const int hole_suit_offset = hole_suit_offsets[board_suit_pattern][hole_suit];
    assert(hole_suit_offset != -1);

    const int offset = board_rank_suit_offset + hole_suit_offset;

    return board_rank_pattern_offsets_[board_rank_pattern] +
        offset * board_rank_combinations_[board_rank_pattern] * 91 +
        board_rank_offset * 91 +
        hole_rank;
}

void holdem_river_lut::init()
{
    std::memset(&rank_indexes_[0], 0, sizeof(rank_indexes_[0][0]) * rank_indexes_.size() * rank_indexes_[0].size());

    board_rank_combinations_.fill(0);
    int index = 0;

    for (int i = 0; i < 13; ++i)
    {
        rank_indexes_[3][i] = index - rank_indexes_[2][i] - rank_indexes_[1][i] - rank_indexes_[0][i] - i;

        for (int j = i; j < 13; ++j)
        {
            if (i == 0)
                rank_indexes_[2][j] = index - rank_indexes_[1][j] - rank_indexes_[0][j] - j;

            for (int k = j; k < 13; ++k)
            {
                if (j == 0)
                    rank_indexes_[1][k] = index - rank_indexes_[0][k] - k;

                for (int l = k; l < 13; ++l)
                {
                    if (k == 0)
                        rank_indexes_[0][l] = index - l;

                    for (int m = l; m < 13; ++m)
                    {
                        int rank_pattern;

                        if (i == j) // XX...
                        {
                            if (j == k) // XXX..
                            {
                                if (k == l) // XXXX. (XXXXX impossible)
                                    rank_pattern = (l == m) ? -1 : RANK_XXXXY;
                                else // XXXY.
                                    rank_pattern = (l == m) ? RANK_XXXYY : RANK_XXXYZ;
                            }
                            else // XXY..
                            {
                                if (k == l) // XXYY.
                                    rank_pattern = (l == m) ? RANK_XXYYY : RANK_XXYYZ;
                                else // XXYZ.
                                    rank_pattern = (l == m) ? RANK_XXYZZ : RANK_XXYZU;
                            }
                        }
                        else // XY...
                        {
                            if (j == k) // XYY..
                            {
                                if (k == l) // XYYY.
                                    rank_pattern = (l == m) ? RANK_XYYYY : RANK_XYYYZ;
                                else // XYYZ.
                                    rank_pattern = (l == m) ? RANK_XYYZZ : RANK_XYYZU;
                            }
                            else // XYZ..
                            {
                                if (k == l) // XYZZ.
                                    rank_pattern = (l == m) ? RANK_XYZZZ : RANK_XYZZU;
                                else // XYZU.
                                    rank_pattern = (l == m) ? RANK_XYZUU : RANK_XYZUV;
                            }
                        }

                        board_rank_patterns_.push_back(std::make_pair(rank_pattern,
                            rank_pattern != -1 ? board_rank_combinations_[rank_pattern]++ : -1));
                        ++index;
                    }
                }
            }
        }
    }

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                for (int l = 0; l < 4; ++l)
                {
                    for (int m = 0; m < 4; ++m)
                    {
                        int suit_pattern = -1;

                        if (j == i) // AA...
                        {
                            if (k == i) // AAA..
                            {
                                if (l == i) // AAAA.
                                    suit_pattern = (m == i) ? SUIT_AAAAA : SUIT_AAAAO;
                                else // AAAO.
                                    suit_pattern = (m == i) ? SUIT_AAAOA : SUIT_AAAOO;
                            }
                            else // AAO..
                            {
                                if (l == i) // AAOA.
                                    suit_pattern = (m == i) ? SUIT_AAOAA : SUIT_AAOAO;
                                else if (m == i)
                                    suit_pattern = SUIT_AAOOA;
                            }
                        }
                        else // AO...
                        {
                            if (k == i) // AOA..
                            {
                                if (l == i) // AOAA.
                                    suit_pattern = (m == i) ? SUIT_AOAAA : SUIT_AOAAO;
                                else if (m == i)
                                    suit_pattern = SUIT_AOAOA;
                            }
                            else // AOO..
                            {
                                if (l == i && m == i)
                                    suit_pattern = SUIT_AOOAA;
                            }
                        }

                        if (suit_pattern == -1)
                        {
                            if (k == j) // OAA..
                            {
                                if (l == j) // OAAA.
                                    suit_pattern = (m == j) ? SUIT_OAAAA : SUIT_OAAAO;
                                else if (m == j)
                                    suit_pattern = SUIT_OAAOA;
                            }
                            else if (l == j && m == j)
                                suit_pattern = SUIT_OAOAA;
                            else if (l == k && m == k)
                                suit_pattern = SUIT_OOAAA;
                        }

                        if (suit_pattern == -1)
                            suit_pattern = SUIT_OOOOO;

                        board_suit_patterns_.push_back(suit_pattern);
                    }
                }
            }
        }
    }

    int size = 0;

    for (int i = 0; i < RANK_COUNT; ++i)
    {
        board_rank_pattern_offsets_[i] = size;

        int offset = 0;

        for (int j = 0; j < SUIT_COUNT; ++j)
        {
            if (board_rank_suits[i][j])
            {
                board_rank_suit_offsets_[i][j] = offset;

                switch (j)
                {
                case SUIT_OOOOO:
                    offset += 1; // OO
                    break;
                default:
                    offset += 4; // AA, AO, OA, OO
                    break;
                }
            }
            else
            {
                board_rank_suit_offsets_[i][j] = -1;
            }
        }

        size += board_rank_combinations_[i] * 91 * offset;
    }

    data_.resize(size);
}
