#include "gtest/gtest.h"
#include "lutlib/holdem_river_ochs_lut.h"
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

TEST(holdem_river_ochs_lut, known_values)
{
    holdem_river_ochs_lut lut(std::string(test::TEST_DATA_PATH) + "/holdem_river_ochs_lut.dat");

    auto data = lut.get_data(get_cards("Tc8d3h4hTh2s3s"));

    EXPECT_NEAR(0.59420, data[0], 0.00001);
    EXPECT_NEAR(0.78947, data[1], 0.00001);
    EXPECT_NEAR(0.84959, data[2], 0.00001);
    EXPECT_NEAR(0.84663, data[3], 0.00001);
    EXPECT_NEAR(0.71905, data[4], 0.00001);
    EXPECT_NEAR(0.79070, data[5], 0.00001);
    EXPECT_NEAR(0.80142, data[6], 0.00001);
    EXPECT_NEAR(0.26471, data[7], 0.00001);

    data = lut.get_data(get_cards("Jc2d5h9hQh2s3s"));

    EXPECT_NEAR(0.26897, data[0], 0.00001);
    EXPECT_NEAR(0.32143, data[1], 0.00001);
    EXPECT_NEAR(0.59589, data[2], 0.00001);
    EXPECT_NEAR(0.33333, data[3], 0.00001);
    EXPECT_NEAR(0.30303, data[4], 0.00001);
    EXPECT_NEAR(0.49432, data[5], 0.00001);
    EXPECT_NEAR(0.53676, data[6], 0.00001);
    EXPECT_NEAR(0.00000, data[7], 0.00001);

    data = lut.get_data(get_cards("8c7d2h3h4h2s4s"));

    EXPECT_NEAR(0.00000, data[0], 0.00001);
    EXPECT_NEAR(0.11441, data[1], 0.00001);
    EXPECT_NEAR(0.18077, data[2], 0.00001);
    EXPECT_NEAR(0.00000, data[3], 0.00001);
    EXPECT_NEAR(0.00000, data[4], 0.00001);
    EXPECT_NEAR(0.00000, data[5], 0.00001);
    EXPECT_NEAR(0.00000, data[6], 0.00001);
    EXPECT_NEAR(0.00000, data[7], 0.00001);
}
