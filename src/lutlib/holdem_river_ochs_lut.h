#pragma once

#include <vector>
#include <array>
#include <memory>
#include "hand_indexer.h"

class holdem_river_ochs_lut
{
public:
    typedef std::array<float, 8> data_type;

    holdem_river_ochs_lut();
    holdem_river_ochs_lut(const std::string& filename);
    void save(const std::string& filename) const;
    const data_type& get_data(const std::array<card_t, 7>& cards) const;
    const std::vector<data_type>& get_data() const;
    hand_indexer::hand_index_t get_key(const std::array<card_t, 7>& cards) const;

private:
    std::vector<data_type> data_;
    std::unique_ptr<hand_indexer> indexer_;
};
