#include "gtest/gtest.h"
#include "util/binary_io.h"

TEST(binary_io, pod_types)
{
    const auto filename = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    auto f = binary_open(filename, "wb");

    ASSERT_TRUE(f != nullptr);

    int val = 123;
    double dval = -55.321;

    binary_write(*f, val);
    binary_write(*f, dval);

    f = binary_open(filename, "rb");

    ASSERT_TRUE(f != nullptr);

    binary_read(*f, val);
    binary_read(*f, dval);

    EXPECT_EQ(val, 123);
    EXPECT_EQ(dval, -55.321);
}

TEST(binary_io, vector)
{
    const auto filename = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    auto f = binary_open(filename, "wb");

    ASSERT_TRUE(f != nullptr);

    std::vector<int> v;
    v.push_back(4);
    v.push_back(-3);
    v.push_back(12);
    v.push_back(200);

    binary_write(*f, v);

    v.clear();

    f = binary_open(filename, "rb");

    ASSERT_TRUE(f != nullptr);

    binary_read(*f, v);

    EXPECT_EQ(v[0], 4);
    EXPECT_EQ(v[1], -3);
    EXPECT_EQ(v[2], 12);
    EXPECT_EQ(v[3], 200);
}

TEST(binary_io, array)
{
    const auto filename = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    auto f = binary_open(filename, "wb");

    ASSERT_TRUE(f != nullptr);

    std::array<int, 5> v = {{7, -54, 9019, -444, 345}};

    binary_write(*f, v);

    v.fill(0);

    f = binary_open(filename, "rb");

    ASSERT_TRUE(f != nullptr);

    binary_read(*f, v);

    EXPECT_EQ(v[0], 7);
    EXPECT_EQ(v[1], -54);
    EXPECT_EQ(v[2], 9019);
    EXPECT_EQ(v[3], -444);
    EXPECT_EQ(v[4], 345);
}
