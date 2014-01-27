#pragma once

#include <vector>
#include <memory>
#include "hand_indexer.h"

class holdem_river_lut
{
public:
    typedef float data_type;
    typedef hand_indexer::hand_index_t index_t;

    holdem_river_lut();
    holdem_river_lut(const std::string& filename);
    void save(const std::string& filename) const;
    const data_type& get(const std::array<int, 7>& cards) const;
    index_t get_key(const std::array<int, 7>& cards) const;

private:
    std::vector<data_type> data_;
    std::unique_ptr<hand_indexer> indexer_;
};
