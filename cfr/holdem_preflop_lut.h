#pragma once

#include <utility>
#include <array>

class holdem_preflop_lut
{
public:
    typedef std::pair<float, float> data_type;

    holdem_preflop_lut();
    holdem_preflop_lut(std::istream&& is);
    void save(std::ostream& os) const;
    const data_type& get(int c0, int c1) const;
    int get_key(int c0, int c1) const;

private:

    std::array<data_type, 169> data_;
};
