#pragma once

#include <vector>
#include <array>

class holdem_flop_lut
{
public:
    enum
    {
        RANK_XXX,
        RANK_XXY,
        RANK_XYY,
        RANK_XYZ,
        RANK_COUNT
    };

    enum
    {
        SUIT_AAA,
        SUIT_AAB,
        SUIT_ABA,
        SUIT_ABB,
        SUIT_ABC,
        SUIT_COUNT
    };

    typedef std::pair<float, float> data_type;

    holdem_flop_lut();
    holdem_flop_lut(const std::string& filename);
    void save(const std::string& filename) const;
    const data_type& get(int c0, int c1, int b0, int b1, int b2) const;
    int get_key(int c0, int c1, int b0, int b1, int b2) const;

private:
    void init();

    mutable std::vector<data_type> data_;
    std::array<std::array<int, 13>, 2> rank_indexes_;
    std::vector<std::pair<int, int>> rank_patterns_;
    std::vector<int> suit_patterns_;
    std::array<std::array<int, SUIT_COUNT>, RANK_COUNT> board_rank_suit_offsets_;
    std::array<int, RANK_COUNT> board_rank_combinations_;
    std::array<int, RANK_COUNT> board_rank_pattern_offsets_;
};
