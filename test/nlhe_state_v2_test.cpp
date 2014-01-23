#include "gtest/gtest.h"
#include "cfrlib/nlhe_state_v2.h"

TEST(nlhe_state_v2, raise_translation)
{
    const nlhe_state_v2<
        nlhe_state_base::F_MASK |
        nlhe_state_base::C_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::P_MASK |
        nlhe_state_base::A_MASK> root(10);

    auto state = root.raise(0.5);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_action(state->get_action_index()), nlhe_state_base::RAISE_H);

    state = root.raise(1.0);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_action(state->get_action_index()), nlhe_state_base::RAISE_P);

    state = root.raise(0.75);

    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->get_action(state->get_action_index()) == nlhe_state_base::RAISE_H
        || state->get_action(state->get_action_index()) == nlhe_state_base::RAISE_P);
}

TEST(nlhe_state_v2, quarter_pot_first_act)
{
    const nlhe_state_v2<
        nlhe_state_base::F_MASK |
        nlhe_state_base::C_MASK |
        nlhe_state_base::O_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::P_MASK |
        nlhe_state_base::A_MASK> root(10);

    auto state = root.get_child(root.get_action_index(nlhe_state_base::RAISE_O));
    
    EXPECT_EQ(state, nullptr);

    state = root.raise(0.25);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_action(state->get_action_index()), nlhe_state_base::RAISE_H);
}

TEST(nlhe_state_v2, pot_sizes)
{
    const nlhe_state_v2<
        nlhe_state_base::F_MASK |
        nlhe_state_base::C_MASK |
        nlhe_state_base::O_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::P_MASK |
        nlhe_state_base::A_MASK> root(10);

    auto state = &root;

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 1);
    EXPECT_EQ(state->get_pot()[1], 2);

    state = state->raise(0.5);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 4);
    EXPECT_EQ(state->get_pot()[1], 2);

    state = state->raise(0.25);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 4);
    EXPECT_EQ(state->get_pot()[1], 6);

    state = state->raise(1.0);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_action(state->get_action_index()), nlhe_state_base::RAISE_A);
    EXPECT_EQ(state->get_pot()[0], 10);
    EXPECT_EQ(state->get_pot()[1], 6);
}
