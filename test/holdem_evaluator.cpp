#include <memory>
#include "gtest/gtest.h"
#include "evallib/holdem_evaluator.h"
#include "util/card.h"

namespace
{
    int get_hand_value(const holdem_evaluator& eval, const std::string& cards)
    {
        return eval.get_hand_value(
            string_to_card(cards.substr(0, 2)),
            string_to_card(cards.substr(2, 2)),
            string_to_card(cards.substr(4, 2)),
            string_to_card(cards.substr(6, 2)),
            string_to_card(cards.substr(8, 2)),
            string_to_card(cards.substr(10, 2)),
            string_to_card(cards.substr(12, 2)));
    }
}

TEST(holdem_evaluator, relative_values)
{
    std::unique_ptr<holdem_evaluator> eval(new holdem_evaluator);

    EXPECT_LT(get_hand_value(*eval, "2c4s6s7s6h3cJs"), get_hand_value(*eval, "AcJd6s7s6h3cJs"));
    EXPECT_LT(get_hand_value(*eval, "AcJd6s7s6h3cJs"), get_hand_value(*eval, "Jc7d6s7s6h3cJs"));
    EXPECT_LT(get_hand_value(*eval, "Jc7d6s7s6h3cJs"), get_hand_value(*eval, "As6d6s7s6h3cJs"));
    EXPECT_LT(get_hand_value(*eval, "As6d6s7s6h3cJs"), get_hand_value(*eval, "4s5d6s7s6h3cJs"));
    EXPECT_LT(get_hand_value(*eval, "4s5d6s7s6h3cJs"), get_hand_value(*eval, "AsKs6s7s6h3cJs"));
    EXPECT_LT(get_hand_value(*eval, "AsKs6s7s6h3cJs"), get_hand_value(*eval, "JdJc6s7s6h3cJs"));
    EXPECT_LT(get_hand_value(*eval, "JdJc6s7s6h3cJs"), get_hand_value(*eval, "6d6c6s7s6h3cJs"));
}

TEST(holdem_evaluator, test_ranks)
{
    std::unique_ptr<holdem_evaluator> eval(new holdem_evaluator);

    int count = 0;
    std::array<int, 10> types = {{}};

    for (int c0 = 0; c0 < 52; ++c0)
    {
	    for (int c1 = c0 + 1; c1 < 52; ++c1)
        {
		    for (int c2 = c1 + 1; c2 < 52; ++c2)
            {
			    for (int c3 = c2 + 1; c3 < 52; ++c3)
                {
				    for (int c4 = c3 + 1; c4 < 52; ++c4)
                    {
					    for (int c5 = c4 + 1; c5 < 52; ++c5)
                        {
						    for (int c6 = c5 + 1; c6 < 52; ++c6)
                            {
							    ++types[eval->get_hand_value(c0, c1, c2, c3, c4, c5, c6) >> 12];
							    ++count;
						    }
					    }
				    }
			    }
		    }
	    }
    }

    EXPECT_EQ(0, types[0]);
    EXPECT_EQ(23294460, types[1]);
    EXPECT_EQ(58627800, types[2]);
    EXPECT_EQ(31433400, types[3]);
    EXPECT_EQ(6461620, types[4]);
    EXPECT_EQ(6180020, types[5]);
    EXPECT_EQ(4047644 , types[6]);
    EXPECT_EQ(3473184, types[7]);
    EXPECT_EQ(224848, types[8]);
    EXPECT_EQ(41584, types[9]);
    EXPECT_EQ(133784560, count);
}
