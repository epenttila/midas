#include "gtest/gtest.h"
#include "lutlib/holdem_river_lut.h"
#include "config.h"

namespace
{
    std::array<int, 7> get_cards(const std::string& cards)
    {
        std::array<int, 7> c = {{
            string_to_card(cards.substr(0, 2)),
            string_to_card(cards.substr(2, 2)),
            string_to_card(cards.substr(4, 2)),
            string_to_card(cards.substr(6, 2)),
            string_to_card(cards.substr(8, 2)),
            string_to_card(cards.substr(10, 2)),
            string_to_card(cards.substr(12, 2))
        }};

        return c;
    }
}

TEST(holdem_river_lut, known_values)
{
    holdem_river_lut lut(std::string(test::TEST_DATA_PATH) + "/holdem_river_lut.dat");

    EXPECT_NEAR(0.84545, lut.get(get_cards("9s8dTd5h9hKd8h")), 0.00001);
    EXPECT_NEAR(0.67677, lut.get(get_cards("Th4d6d5d7sTdQh")), 0.00001);
    EXPECT_NEAR(0.71616, lut.get(get_cards("Kd9h5h6hKhAc7c")), 0.00001);
    EXPECT_NEAR(0.91212, lut.get(get_cards("Qd5d5c7hKd8cQh")), 0.00001);
    EXPECT_NEAR(1.00000, lut.get(get_cards("JdJsThJc6cJhAc")), 0.00001);
    EXPECT_NEAR(0.00000, lut.get(get_cards("2d2s3c3d3h3s2c")), 0.00001);
}
