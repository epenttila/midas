#include "gtest/gtest.h"
#include "util/sort.h"

TEST(sort, sort_2)
{
    int a = 12;
    int b = 5;

    sort(a, b);

    EXPECT_EQ(5, a);
    EXPECT_EQ(12, b);

    a = -43;
    b = -12;

    sort(a, b);

    EXPECT_EQ(-43, a);
    EXPECT_EQ(-12, b);

    a = 31;
    b = -2;

    sort(a, b);

    EXPECT_EQ(-2, a);
    EXPECT_EQ(31, b);
}

TEST(sort, sort_array)
{
    std::array<int, 3> data_3 = {{14, -7, 2}};

    sort(data_3);

    EXPECT_EQ(-7, data_3[0]);
    EXPECT_EQ(2, data_3[1]);
    EXPECT_EQ(14, data_3[2]);
    
    std::array<int, 4> data_4 = {{14, 56, -7, 2}};

    sort(data_4);

    EXPECT_EQ(-7, data_4[0]);
    EXPECT_EQ(2, data_4[1]);
    EXPECT_EQ(14, data_4[2]);
    EXPECT_EQ(56, data_4[3]);
}
