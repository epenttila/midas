#pragma once

#include <istream>
#include <vector>
#include <array>

class holdem_river_lut
{
public:
    enum
    {
        RANK_XXXXY,
        RANK_XXXYY,
        RANK_XXYYY,
        RANK_XYYYY,
        RANK_XXXYZ,
        RANK_XXYYZ,
        RANK_XXYZZ,
        RANK_XYYYZ,
        RANK_XYYZZ,
        RANK_XYZZZ,
        RANK_XXYZU,
        RANK_XYYZU,
        RANK_XYZZU,
        RANK_XYZUU,
        RANK_XYZUV,
        RANK_COUNT
    };

    enum
    {
        SUIT_AAAAA,
        SUIT_AAAAO,
        SUIT_AAAOA,
        SUIT_AAOAA,
        SUIT_AOAAA,
        SUIT_OAAAA,
        SUIT_AAAOO,
        SUIT_AAOAO,
        SUIT_AAOOA,
        SUIT_AOAAO,
        SUIT_AOAOA,
        SUIT_AOOAA,
        SUIT_OAAAO,
        SUIT_OAAOA,
        SUIT_OAOAA,
        SUIT_OOAAA,
        SUIT_OOOOO,
        SUIT_COUNT
    };

    typedef float data_type;

    holdem_river_lut();
    holdem_river_lut(std::istream&& is);
    void save(std::ostream& os) const;
    const data_type& get(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const;
    int get_key(int c0, int c1, int b0, int b1, int b2, int b3, int b4) const;

private:
    void init();

    mutable std::vector<data_type> data_;
    std::array<std::array<int, 13>, 4> rank_indexes_;
    std::vector<std::pair<int, int>> board_rank_patterns_;
    std::vector<int> board_suit_patterns_;
    std::array<std::array<int, SUIT_COUNT>, RANK_COUNT> board_rank_suit_offsets_;
    std::array<int, RANK_COUNT> board_rank_combinations_;
    std::array<int, RANK_COUNT> board_rank_pattern_offsets_;
};
