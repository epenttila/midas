#include "holdem_flop_lut.h"
#include <omp.h>
#include <iostream>
#include <boost/format.hpp>
#include "evallib/holdem_evaluator.h"
#include "util/sort.h"
#include "util/card.h"
#include "util/holdem_loops.h"

namespace
{
    static const bool board_rank_suits[holdem_flop_lut::RANK_COUNT][holdem_flop_lut::SUIT_COUNT] =
    {
         0, 0, 0, 0, 1, // XXX (ABC)
         0, 0, 1, 1, 1, // XXY (ABA, ABB, ABC)
         0, 1, 1, 0, 1, // XYY (AAB, ABA, ABC)
         1, 1, 1, 1, 1, // XYZ (*)
    };

    static const int hole_suit_offsets[holdem_flop_lut::SUIT_COUNT][16] =
    {
        0, -1, -1,  1, -1, -1, -1, -1, -1, -1, -1, -1,  2, -1, -1,  3,
        0,  1, -1,  2,  3,  4, -1,  5, -1, -1, -1, -1,  6,  7, -1,  8,
        0,  1, -1,  2,  3,  4, -1,  5, -1, -1, -1, -1,  6,  7, -1,  8,
        0,  1, -1,  2,  3,  4, -1,  5, -1, -1, -1, -1,  6,  7, -1,  8,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    };
}

holdem_flop_lut::holdem_flop_lut()
{
    init();

    holdem_evaluator e;
    std::vector<bool> generated(data_.size());

    const double start_time = omp_get_wtime();
    double time = start_time;
    int iteration = 0;
    int keys = 0;

#pragma omp parallel
    {
        parallel_for_each_flop([&](int a, int b, int i, int j, int k) { 
            const auto key = get_key(a, b, i, j, k);

            if (!generated[key])
            {
                generated[key] = true;
                const result r = e.enumerate_flop(a, b, i, j, k);
                data_[key] = std::make_pair(float(r.ehs), float(r.ehs2));
#pragma omp atomic
                ++keys;
            }

#pragma omp atomic
            ++iteration;

            const double t = omp_get_wtime();

            if (iteration == 25989600 || (omp_get_thread_num() == 0 && t - time >= 1))
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

holdem_flop_lut::holdem_flop_lut(std::istream&& is)
{
    if (!is)
        throw std::runtime_error("bad istream");

    init();

    is.read(reinterpret_cast<char*>(&data_[0]), sizeof(data_type) * data_.size());

    if (!is)
        throw std::runtime_error("read failed");
}

void holdem_flop_lut::save(std::ostream& os) const
{
    os.write(reinterpret_cast<const char*>(&data_[0]), sizeof(data_type) * data_.size());
}

const std::pair<float, float>& holdem_flop_lut::get(int c0, int c1, int b0, int b1, int b2) const
{
    return data_[get_key(c0, c1, b0, b1, b2)];
}

int holdem_flop_lut::get_key(const int c0, const int c1, const int b0, const int b1, const int b2) const
{
    assert(c0 != c1 && c1 != b0 && b0 != b1 && b1 != b2);

    const int board_cards = 3;
    std::array<int, board_cards> board = {{b0, b1, b2}};

    sort(board);

    const std::array<int, board_cards> board_ranks = {{get_rank(board[0]), get_rank(board[1]), get_rank(board[2])}};
    const std::array<int, board_cards> board_suits = {{get_suit(board[0]), get_suit(board[1]), get_suit(board[2])}};

    // board rank pattern and offset
    const int board_rank = rank_indexes_[1][board_ranks[0]] + rank_indexes_[0][board_ranks[1]] + board_ranks[2];
    const int board_rank_pattern = rank_patterns_[board_rank].first;
    const int board_rank_offset = rank_patterns_[board_rank].second;

    // board suit pattern
    const int board_suit_pattern = suit_patterns_[board_suits[0] * 16 + board_suits[1] * 4 + board_suits[2]];

    std::array<int, 4> suit_map = {{-1, -1, -1, -1}};
    int isomorphic_suit = 0;

    for (int i = 0; i < board.size(); ++i)
    {
        const int suit = board_suits[i];

        if (suit_map[suit] == -1)
            suit_map[suit] = isomorphic_suit++;
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
    const int hole_suit = (hole_iso_suits[0] & 0x3) * 4 + (hole_iso_suits[1] & 0x3); // map -1 ("other" suit)

    const int hole_suit_offset = hole_suit_offsets[board_suit_pattern][hole_suit];
    assert(hole_suit_offset != -1);

    const int offset = board_rank_suit_offset + hole_suit_offset;

    return board_rank_pattern_offsets_[board_rank_pattern] +
        offset * board_rank_combinations_[board_rank_pattern] * 91 +
        board_rank_offset * 91 +
        hole_rank;
}

void holdem_flop_lut::init()
{
    std::memset(&rank_indexes_[0], 0, sizeof(rank_indexes_[0][0]) * rank_indexes_.size() * rank_indexes_[0].size());

    board_rank_combinations_.fill(0);
    int index = 0;

    for (int i = 0; i < 13; ++i)
    {
        rank_indexes_[1][i] = index - rank_indexes_[0][i] - i;

        for (int j = i; j < 13; ++j)
        {
            if (i == 0)
                rank_indexes_[0][j] = index - j;

            for (int k = j; k < 13; ++k)
            {
                int rank_pattern;

                if (i == j)
                    rank_pattern = (j == k) ? RANK_XXX : RANK_XXY;
                else
                    rank_pattern = (j == k) ? RANK_XYY : RANK_XYZ;

                rank_patterns_.push_back(std::make_pair(rank_pattern, board_rank_combinations_[rank_pattern]++));
                ++index;
            }
        }
    }

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                int suit_pattern;

                if (i == j)
                    suit_pattern = (j == k) ? SUIT_AAA : SUIT_AAB;
                else
                    suit_pattern = (i == k) ? SUIT_ABA : (j == k) ? SUIT_ABB : SUIT_ABC;

                suit_patterns_.push_back(suit_pattern);
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
                case SUIT_AAA:
                    offset += 4; // AA, AO, OA, OO
                    break;
                case SUIT_ABC:
                    offset += 16; // AA, AB, AC, AO, BA, BB, BC, BO, CA, CB, CC, CO, OA, OB, OC, OO
                    break;
                default:
                    offset += 9; // AA, AB, AO, BA, BB, BO, OA, OB, OO
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
