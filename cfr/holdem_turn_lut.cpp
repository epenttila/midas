#include "holdem_turn_lut.h"
#include <omp.h>
#include <iostream>
#include <boost/format.hpp>
#include "holdem_evaluator.h"
#include "compare_and_swap.h"
#include "card.h"

namespace
{
    static const bool board_rank_suits[holdem_turn_lut::RANK_COUNT][holdem_turn_lut::SUIT_COUNT] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, // XXXX
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, // XXXY
        0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, // XXYY
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, // XYYY
        0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, // XXYZ
        0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, // XYYZ
        0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, // XYZZ
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // XYZU
    };

    static const int hole_suit_offsets[holdem_turn_lut::SUIT_COUNT][9] =
    {
         0, -1,  1, -1, -1, -1,  2, -1,  3,
         0, -1,  1, -1, -1, -1,  2, -1,  3,
         0, -1,  1, -1, -1, -1,  2, -1,  3,
         0, -1,  1, -1, -1, -1,  2, -1,  3,
         0, -1,  1, -1, -1, -1,  2, -1,  3,
         0,  1,  2,  3,  4,  5,  6,  7,  8,
         0,  1,  2,  3,  4,  5,  6,  7,  8,
         0,  1,  2,  3,  4,  5,  6,  7,  8,
         0, -1,  1, -1, -1, -1,  2, -1,  3,
         0, -1,  1, -1, -1, -1,  2, -1,  3,
         0, -1,  1, -1, -1, -1,  2, -1,  3,
         0, -1,  1, -1, -1, -1,  2, -1,  3,
         0, -1,  1, -1, -1, -1,  2, -1,  3,
         0, -1,  1, -1, -1, -1,  2, -1,  3,
        -1, -1, -1, -1, -1, -1, -1, -1,  0,
    };
}

holdem_turn_lut::holdem_turn_lut()
{
    init();

    holdem_evaluator e;
    std::vector<bool> generated(data_.size());

    const double start_time = omp_get_wtime();
    double time = start_time;
    int iteration = 0;
    int keys = 0;

#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < 49; ++i)
    {
        for (int j = i + 1; j < 50; ++j)
        {
            for (int k = j + 1; k < 51; ++k)
            {
                for (int l = k + 1; l < 52; ++l)
                {
                    for (int a = 0; a < 51; ++a)
                    {
                        if (a == i || a == j || a == k || a == l)
                            continue;

                        for (int b = a + 1; b < 52; ++b)
                        {
                            if (b == i || b == j || b == k || b == l)
                                continue;

                            const auto key = get_key(a, b, i, j, k, l);

                            if (!generated[key])
                            {
                                generated[key] = true;
                                const result r = e.enumerate_turn(a, b, i, j, k, l);
                                data_[key] = std::make_pair(float(r.ehs), float(r.ehs2));
#pragma omp atomic
                                ++keys;
                            }

#pragma omp atomic
                            ++iteration;

                            const double t = omp_get_wtime();

                            if (iteration == 305377800 || (omp_get_thread_num() == 0 && t - time >= 1))
                            {
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
            }
        }
    }
}

holdem_turn_lut::holdem_turn_lut(std::istream&& is)
{
    if (!is)
        throw std::runtime_error("bad istream");

    init();
    is.read(reinterpret_cast<char*>(&data_[0]), sizeof(data_type) * data_.size());

    if (!is)
        throw std::runtime_error("read failed");
}

void holdem_turn_lut::save(std::ostream& os) const
{
    os.write(reinterpret_cast<const char*>(&data_[0]), sizeof(data_type) * data_.size());
}

const std::pair<float, float>& holdem_turn_lut::get(int c0, int c1, int b0, int b1, int b2, int b3) const
{
    return data_[get_key(c0, c1, b0, b1, b2, b3)];
}

int holdem_turn_lut::get_key(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3) const
{
    const int board_cards = 4;
    std::array<int, board_cards> board = {{b0, b1, b2, b3}};

    compare_and_swap(board[0], board[1]);
    compare_and_swap(board[2], board[3]);
    compare_and_swap(board[0], board[2]);
    compare_and_swap(board[1], board[3]);
    compare_and_swap(board[1], board[2]);

    const std::array<int, board_cards> board_ranks = {{get_rank(board[0]), get_rank(board[1]), get_rank(board[2]), get_rank(board[3])}};
    const std::array<int, board_cards> board_suits = {{get_suit(board[0]), get_suit(board[1]), get_suit(board[2]), get_suit(board[3])}};

    // board rank pattern and offset
    const int board_rank = rank_indexes_[2][board_ranks[0]] + rank_indexes_[1][board_ranks[1]] + rank_indexes_[0][board_ranks[2]] + board_ranks[3];
    const int board_rank_pattern = board_rank_patterns_[board_rank].first;
    const int board_rank_offset = board_rank_patterns_[board_rank].second;

    // board suit pattern
    const int board_suit_pattern = board_suit_patterns_[board_suits[0] * 64 + board_suits[1] * 16 + board_suits[2] * 4 + board_suits[3]];

    // suit mapping base 3 (P=0, S=1, O=2)
    std::array<int, 4> suit_counts = {{}};

    for (int i = 0; i < board.size(); ++i)
        ++suit_counts[board_suits[i]];

    std::array<int, 4> suit_map = {{2, 2, 2, 2}};
    int isomorphic_suit = 0;

    for (int i = 0; i < suit_counts.size(); ++i)
    {
        if (suit_counts[i] >= 3)
            suit_map[i] = 0;
        else if (suit_counts[i] == 2)
            suit_map[i] = isomorphic_suit++;
    }

    // board+hole suit combo counts
    const int board_rank_suit_offset = board_rank_suit_offsets_[board_rank_pattern][board_suit_pattern];
    assert(board_rank_suit_offset != -1);

    // hole
    std::array<int, 2> hole = {{c0, c1}};
    compare_and_swap(hole[0], hole[1]);

    const std::array<int, 2> hole_ranks = {{get_rank(hole[0]), get_rank(hole[1])}};
    const std::array<int, 2> hole_iso_suits = {{suit_map[get_suit(hole[0])], suit_map[get_suit(hole[1])]}};

    const int hole_rank = rank_indexes_[0][hole_ranks[0]] + hole_ranks[1];
    const int hole_suit = hole_iso_suits[0] * 3 + hole_iso_suits[1];

    const int hole_suit_offset = hole_suit_offsets[board_suit_pattern][hole_suit];
    assert(hole_suit_offset != -1);

    const int offset = board_rank_suit_offset + hole_suit_offset;

    return board_rank_pattern_offsets_[board_rank_pattern] +
        offset * board_rank_combinations_[board_rank_pattern] * 91 +
        board_rank_offset * 91 +
        hole_rank;
}

void holdem_turn_lut::init()
{
    std::memset(&rank_indexes_[0], 0, sizeof(rank_indexes_[0][0]) * rank_indexes_.size() * rank_indexes_[0].size());

    board_rank_combinations_.fill(0);
    int index = 0;

    for (int i = 0; i < 13; ++i)
    {
        rank_indexes_[2][i] = index - rank_indexes_[1][i] - rank_indexes_[0][i] - i;

        for (int j = i; j < 13; ++j)
        {
            if (i == 0)
                rank_indexes_[1][j] = index - rank_indexes_[0][j] - j;

            for (int k = j; k < 13; ++k)
            {
                if (j == 0)
                    rank_indexes_[0][k] = index - k;

                for (int l = k; l < 13; ++l)
                {
                    int rank_pattern;

                    if (i == j) // XX..
                    {
                        if (j == k) // XXX.
                            rank_pattern = (k == l) ? RANK_XXXX : RANK_XXXY;
                        else // XXY.
                            rank_pattern = (k == l) ? RANK_XXYY : RANK_XXYZ;
                    }
                    else // XY..
                    {
                        if (j == k) // XYY.
                            rank_pattern = (k == l) ? RANK_XYYY : RANK_XYYZ;
                        else // XYZ.
                            rank_pattern = (k == l) ? RANK_XYZZ : RANK_XYZU;
                    }

                    board_rank_patterns_.push_back(std::make_pair(rank_pattern, board_rank_combinations_[rank_pattern]++));
                    ++index;
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
                    int suit_pattern;

                    if (j == i) // AA..
                    {
                        if (k == i) // AAA.
                        {
                            if (l == i)
                                suit_pattern = SUIT_AAAA;
                            else
                                suit_pattern = SUIT_AAAB;
                        }
                        else // AAB.
                        {
                            if (l == i)
                                suit_pattern = SUIT_AABA;
                            else if (l == k)
                                suit_pattern = SUIT_AABB;
                            else
                                suit_pattern = SUIT_AABC;
                        }
                    }
                    else // AB..
                    {
                        if (k == i) // ABA.
                        {
                            if (l == i)
                                suit_pattern = SUIT_ABAA;
                            else if (l == j)
                                suit_pattern = SUIT_ABAB;
                            else
                                suit_pattern = SUIT_ABAC;
                        }
                        else if (k == j) // ABB.
                        {
                            if (l == i)
                                suit_pattern = SUIT_ABBA;
                            else if (l == j)
                                suit_pattern = SUIT_ABBB;
                            else
                                suit_pattern = SUIT_ABBC;
                        }
                        else // ABC.
                        {
                            if (l == i)
                                suit_pattern = SUIT_ABCA;
                            else if (l == j)
                                suit_pattern = SUIT_ABCB;
                            else if (l == k)
                                suit_pattern = SUIT_ABCC;
                            else
                                suit_pattern = SUIT_ABCD;
                        }
                    }

                    board_suit_patterns_.push_back(suit_pattern);
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
                case SUIT_AABB:
                case SUIT_ABAB:
                case SUIT_ABBA:
                    offset += 9; // PP, PS, PO, SP, SS, SO, OP, OS, OO
                    break;
                case SUIT_ABCD:
                    offset += 1; // OO
                    break;
                default:
                    offset += 4; // PP, PO, OP, OO
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
