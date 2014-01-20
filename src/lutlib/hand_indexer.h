#pragma once

#include <string>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include "util/card.h"

struct hand_indexer_state_t;
struct hand_indexer_t;

class hand_indexer
{
public:
    typedef std::uint64_t hand_index_t;

    hand_indexer(const std::vector<card_t>& cards_per_round);
    ~hand_indexer();

    hand_index_t hand_index_last(const card_t cards[]) const;
    bool hand_unindex(int round, hand_index_t index, card_t cards[]) const;
    std::size_t get_size(int round) const;
    int get_rounds() const;

private:
    static const unsigned int MAX_GROUP_INDEX = 0x100000;

    void tabulate_configurations(uint_fast32_t round, uint_fast32_t configuration[], void * data);
    hand_index_t hand_index_next_round(const uint8_t cards[], hand_indexer_state_t * state) const;
    hand_index_t hand_index_all(const uint8_t cards[], hand_index_t indices[]) const;

    std::array<std::array<std::uint8_t, RANKS>, 1 << RANKS> nth_unset;
    std::array<std::array<bool, SUITS>, 1 << (SUITS - 1)> equal;
    std::array<std::array<std::uint32_t, RANKS + 1>, RANKS + 1> nCr_ranks;
    std::array<std::uint32_t, 1 << RANKS> rank_set_to_index;
    std::array<std::array<std::uint32_t, 1 << RANKS>, RANKS + 1> index_to_rank_set;
    //std::uint32_t index_to_rank_set[RANKS][1 << RANKS];
    std::uint_fast32_t (*suit_permutations)[SUITS];
    std::array<std::array<hand_index_t, SUITS + 1>, MAX_GROUP_INDEX> nCr_groups;

    hand_indexer_t* indexer;
};
