#pragma once

#include <utility>
#include <vector>
#include <array>

class holdem_turn_lut
{
public:
    enum
    {
        RANK_XXXX,
        RANK_XXXY,
        RANK_XXYY,
        RANK_XYYY,
        RANK_XXYZ,
        RANK_XYYZ,
        RANK_XYZZ,
        RANK_XYZU,
        RANK_COUNT
    };

    enum
    {
        SUIT_AAAA = 0,
        SUIT_AAAB = 1,
        SUIT_AABA = 2,
        SUIT_ABAA = 3,
        SUIT_ABBB = 4,
        SUIT_AABB = 5,
        SUIT_ABAB = 6,
        SUIT_ABBA = 7,
        SUIT_AABC = 8,
        SUIT_ABAC = 9,
        SUIT_ABCA = 10,
        SUIT_ABBC = 11,
        SUIT_ABCB = 12,
        SUIT_ABCC = 13, /// @todo replace with OOAA as we need two on turn to make flush
        SUIT_ABCD = 14,
        SUIT_COUNT
    };

    typedef std::pair<float, float> data_type;

    holdem_turn_lut();
    holdem_turn_lut(std::istream&& is);
    void save(std::ostream& os) const;
    const data_type& get(int c0, int c1, int b0, int b1, int b2, int b3) const;
    int get_key(int c0, int c1, int b0, int b1, int b2, int b3) const;

private:
    void init();

    mutable std::vector<data_type> data_;
    std::array<std::array<int, 13>, 3> rank_indexes_;
    std::vector<std::pair<int, int>> board_rank_patterns_;
    std::vector<int> board_suit_patterns_;
    std::array<std::array<int, SUIT_COUNT>, RANK_COUNT> board_rank_suit_offsets_;
    std::array<int, RANK_COUNT> board_rank_combinations_;
    std::array<int, RANK_COUNT> board_rank_pattern_offsets_;
};
