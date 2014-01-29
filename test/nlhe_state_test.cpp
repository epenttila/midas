#include "gtest/gtest.h"
#include "gamelib/nlhe_state.h"
#include "util/game.h"

TEST(nlhe_state, raise_translation)
{
    const nlhe_state root(10,
        nlhe_state_base::F_MASK |
        nlhe_state_base::C_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::P_MASK |
        nlhe_state_base::A_MASK, 0);

    auto state = root.raise(0.5);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_action(), nlhe_state_base::RAISE_H);

    state = root.raise(1.0);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_action(), nlhe_state_base::RAISE_P);

    state = root.raise(0.75);

    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->get_action() == nlhe_state_base::RAISE_H
        || state->get_action() == nlhe_state_base::RAISE_P);
}

TEST(nlhe_state, quarter_pot_first_act)
{
    const nlhe_state root(10,
        nlhe_state_base::F_MASK |
        nlhe_state_base::C_MASK |
        nlhe_state_base::O_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::P_MASK |
        nlhe_state_base::A_MASK, 0);

    auto state = root.get_action_child(nlhe_state_base::RAISE_O);
    
    EXPECT_EQ(state, nullptr);

    state = root.raise(0.25);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_action(), nlhe_state_base::RAISE_H);
}

TEST(nlhe_state, pot_sizes)
{
    const nlhe_state root(10, 
        nlhe_state_base::F_MASK |
        nlhe_state_base::C_MASK |
        nlhe_state_base::O_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::P_MASK |
        nlhe_state_base::A_MASK, 0);

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
    EXPECT_EQ(state->get_action(), nlhe_state_base::RAISE_A);
    EXPECT_EQ(state->get_pot()[0], 10);
    EXPECT_EQ(state->get_pot()[1], 6);
}

TEST(nlhe_state, combine_actions)
{
    const nlhe_state root(50,
        nlhe_state_base::F_MASK |
        nlhe_state_base::C_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::P_MASK |
        nlhe_state_base::A_MASK, 0);

    auto state = &root;

    EXPECT_EQ(state->get_round(), PREFLOP);
    EXPECT_EQ(state->get_pot()[0], 1);
    EXPECT_EQ(state->get_pot()[1], 2);

    state = state->call();

    EXPECT_EQ(state->get_round(), PREFLOP);
    EXPECT_EQ(state->get_pot()[0], 2);
    EXPECT_EQ(state->get_pot()[1], 2);

    state = state->call();

    EXPECT_EQ(state->get_round(), FLOP);
    EXPECT_EQ(state->get_pot()[0], 2);
    EXPECT_EQ(state->get_pot()[1], 2);

    state = state->raise(1.0);

    EXPECT_EQ(state->get_round(), FLOP);
    EXPECT_EQ(state->get_pot()[0], 2);
    EXPECT_EQ(state->get_pot()[1], 6);

    state = state->raise(1.0);

    EXPECT_EQ(state->get_round(), FLOP);
    EXPECT_EQ(state->get_pot()[0], 18);
    EXPECT_EQ(state->get_pot()[1], 6);
    EXPECT_EQ(state->get_child_count(), 4);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::FOLD) != nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::CALL) != nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::RAISE_H) != nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::RAISE_P) == nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::RAISE_A) != nullptr);

    state = state->raise(1.5);

    EXPECT_EQ(state->get_round(), FLOP);
    EXPECT_EQ(state->get_pot()[0], 18);
    EXPECT_EQ(state->get_pot()[1], 50);
    EXPECT_EQ(state->get_action(), nlhe_state_base::RAISE_A);

    state = state->call();

    EXPECT_EQ(state->get_round(), TURN);
    EXPECT_EQ(state->get_pot()[0], 50);
    EXPECT_EQ(state->get_pot()[1], 50);
    EXPECT_TRUE(state->is_terminal());
}

TEST(nlhe_state, many_halfpots)
{
    const nlhe_state root(50,
        nlhe_state_base::F_MASK |
        nlhe_state_base::C_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::P_MASK |
        nlhe_state_base::A_MASK, 0);

    auto state = &root;

    ASSERT_TRUE(state != nullptr);
    EXPECT_EQ(state->get_round(), PREFLOP);
    EXPECT_EQ(state->get_pot()[0], 1);
    EXPECT_EQ(state->get_pot()[1], 2);

    state = state->raise(0.5);

    ASSERT_TRUE(state != nullptr);
    EXPECT_EQ(state->get_round(), PREFLOP);
    EXPECT_EQ(state->get_pot()[0], 4);
    EXPECT_EQ(state->get_pot()[1], 2);

    state = state->raise(0.5);

    ASSERT_TRUE(state != nullptr);
    EXPECT_EQ(state->get_round(), PREFLOP);
    EXPECT_EQ(state->get_pot()[0], 4);
    EXPECT_EQ(state->get_pot()[1], 8);

    state = state->raise(0.5);

    ASSERT_TRUE(state != nullptr);
    EXPECT_EQ(state->get_round(), PREFLOP);
    EXPECT_EQ(state->get_pot()[0], 16);
    EXPECT_EQ(state->get_pot()[1], 8);

    state = state->raise(0.5);

    ASSERT_TRUE(state != nullptr);
    EXPECT_EQ(state->get_round(), PREFLOP);
    EXPECT_EQ(state->get_pot()[0], 16);
    EXPECT_EQ(state->get_pot()[1], 32);

    state = state->raise(0.5);

    ASSERT_TRUE(state != nullptr);
    EXPECT_EQ(state->get_round(), PREFLOP);
    EXPECT_EQ(state->get_pot()[0], 50);
    EXPECT_EQ(state->get_pot()[1], 32);
}

TEST(nlhe_state, state_counts_nlhe_fchqpwdta_50)
{
    const nlhe_state root(50,
        nlhe_state_base::F_MASK |
        nlhe_state_base::C_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::Q_MASK |
        nlhe_state_base::P_MASK |
        nlhe_state_base::W_MASK |
        nlhe_state_base::D_MASK |
        nlhe_state_base::T_MASK |
        nlhe_state_base::A_MASK, 0);

    std::vector<const nlhe_state_base*> states;

    for (const auto p : game_state_base::get_state_vector(root))
        states.push_back(dynamic_cast<const nlhe_state_base*>(p));

    EXPECT_EQ(28000u, states.size());

    std::vector<int> counts(RIVER + 1);
    std::for_each(states.begin(), states.end(), [&](const nlhe_state_base* s) { ++counts[s->get_round()]; });

    EXPECT_EQ(336, counts[0]);
    EXPECT_EQ(2112, counts[1]);
    EXPECT_EQ(7248, counts[2]);
    EXPECT_EQ(18304, counts[3]);
}

TEST(nlhe_state, state_counts_nlhe_fcOHQpwdvta_40)
{
    const nlhe_state root(40, 
        nlhe_state_base::F_MASK |
        nlhe_state_base::C_MASK |
        nlhe_state_base::O_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::Q_MASK |
        nlhe_state_base::P_MASK |
        nlhe_state_base::W_MASK |
        nlhe_state_base::D_MASK |
        nlhe_state_base::V_MASK |
        nlhe_state_base::T_MASK |
        nlhe_state_base::A_MASK,
        nlhe_state_base::O_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::Q_MASK);

    std::vector<const nlhe_state_base*> states;

    for (const auto p : game_state_base::get_state_vector(root))
        states.push_back(dynamic_cast<const nlhe_state_base*>(p));

    EXPECT_EQ(83008u, states.size());

    std::vector<int> counts(RIVER + 1);
    std::for_each(states.begin(), states.end(), [&](const nlhe_state_base* s) { ++counts[s->get_round()]; });

    EXPECT_EQ(464, counts[0]);
    EXPECT_EQ(4048, counts[1]);
    EXPECT_EQ(18528, counts[2]);
    EXPECT_EQ(59968, counts[3]);
}

TEST(nlhe_state, minimum_bet_size_preflop)
{
    const nlhe_state root(50,
        nlhe_state_base::F_MASK |
        nlhe_state_base::C_MASK |
        nlhe_state_base::O_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::Q_MASK |
        nlhe_state_base::P_MASK |
        nlhe_state_base::W_MASK |
        nlhe_state_base::D_MASK |
        nlhe_state_base::V_MASK |
        nlhe_state_base::T_MASK |
        nlhe_state_base::A_MASK, 0);

    auto state = &root;

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 1);
    EXPECT_EQ(state->get_pot()[1], 2);
    EXPECT_EQ(state->get_round(), PREFLOP);

    state = state->raise(0.25);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 4);
    EXPECT_EQ(state->get_pot()[1], 2);
    EXPECT_EQ(state->get_round(), PREFLOP);

    state = &root;
    state = state->raise(0.5);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 4);
    EXPECT_EQ(state->get_pot()[1], 2);
    EXPECT_EQ(state->get_round(), PREFLOP);

    state = state->call();

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 4);
    EXPECT_EQ(state->get_pot()[1], 4);
    EXPECT_EQ(state->get_round(), FLOP);

    state = state->call();

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 4);
    EXPECT_EQ(state->get_pot()[1], 4);
    EXPECT_EQ(state->get_round(), FLOP);

    state = state->raise(1.0);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 12);
    EXPECT_EQ(state->get_pot()[1], 4);
    EXPECT_EQ(state->get_round(), FLOP);

    state = state->raise(0.25);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 12);
    EXPECT_EQ(state->get_pot()[1], 20);
    EXPECT_EQ(state->get_round(), FLOP);

    state = state->get_parent();
    state = state->raise(0.5);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 12);
    EXPECT_EQ(state->get_pot()[1], 24);
    EXPECT_EQ(state->get_round(), FLOP);
}

TEST(nlhe_state, minimum_bet_size_postflop)
{
    const nlhe_state root(50,
        nlhe_state_base::F_MASK |
        nlhe_state_base::C_MASK |
        nlhe_state_base::O_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::Q_MASK |
        nlhe_state_base::P_MASK |
        nlhe_state_base::W_MASK |
        nlhe_state_base::D_MASK |
        nlhe_state_base::V_MASK |
        nlhe_state_base::T_MASK |
        nlhe_state_base::A_MASK, 0);

    auto state = &root;

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 1);
    EXPECT_EQ(state->get_pot()[1], 2);
    EXPECT_EQ(state->get_round(), PREFLOP);

    state = state->call();

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 2);
    EXPECT_EQ(state->get_pot()[1], 2);
    EXPECT_EQ(state->get_round(), PREFLOP);

    state = state->call();

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 2);
    EXPECT_EQ(state->get_pot()[1], 2);
    EXPECT_EQ(state->get_round(), FLOP);

    state = state->raise(5.0);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 2);
    EXPECT_EQ(state->get_pot()[1], 22);
    EXPECT_EQ(state->get_round(), FLOP);

    EXPECT_EQ(state->get_child_count(), 5);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::FOLD) != nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::CALL) != nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::RAISE_O) != nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::RAISE_H) != nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::RAISE_P) == nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::RAISE_A) != nullptr);

    state = state->raise(0.25);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 42);
    EXPECT_EQ(state->get_pot()[1], 22);
    EXPECT_EQ(state->get_round(), FLOP);

    state = state->get_parent();
    state = state->raise(0.5);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 44);
    EXPECT_EQ(state->get_pot()[1], 22);
    EXPECT_EQ(state->get_round(), FLOP);

    state = state->get_parent();
    state = state->raise(0.75);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 50);
    EXPECT_EQ(state->get_pot()[1], 22);
    EXPECT_EQ(state->get_round(), FLOP);
}

TEST(nlhe_state, round_up_bets)
{
    const nlhe_state root(50,
        nlhe_state_base::F_MASK |
        nlhe_state_base::C_MASK |
        nlhe_state_base::O_MASK |
        nlhe_state_base::H_MASK |
        nlhe_state_base::Q_MASK |
        nlhe_state_base::P_MASK |
        nlhe_state_base::W_MASK |
        nlhe_state_base::D_MASK |
        nlhe_state_base::V_MASK |
        nlhe_state_base::T_MASK |
        nlhe_state_base::A_MASK, 0);

    auto state = &root;

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 1);
    EXPECT_EQ(state->get_pot()[1], 2);
    EXPECT_EQ(state->get_round(), PREFLOP);

    state = state->call();

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 2);
    EXPECT_EQ(state->get_pot()[1], 2);
    EXPECT_EQ(state->get_round(), PREFLOP);

    EXPECT_TRUE(state->raise(5.0) == state->get_action_child(nlhe_state_base::RAISE_V));

    state = state->get_action_child(nlhe_state_base::RAISE_V);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 2);
    EXPECT_EQ(state->get_pot()[1], 22);
    EXPECT_EQ(state->get_round(), PREFLOP);

    state = state->call();

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 22);
    EXPECT_EQ(state->get_pot()[1], 22);
    EXPECT_EQ(state->get_round(), FLOP);

    EXPECT_TRUE(state->raise(0.25) == state->get_action_child(nlhe_state_base::RAISE_O));

    state = state->get_action_child(nlhe_state_base::RAISE_O);

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 22);
    EXPECT_EQ(state->get_pot()[1], 33);
    EXPECT_EQ(state->get_round(), FLOP);

    state = state->call();

    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get_pot()[0], 33);
    EXPECT_EQ(state->get_pot()[1], 33);
    EXPECT_EQ(state->get_round(), TURN);

    EXPECT_EQ(state->get_child_count(), 2);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::FOLD) == nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::CALL) != nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::RAISE_O) == nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::RAISE_H) == nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::RAISE_P) == nullptr);
    EXPECT_TRUE(state->get_action_child(nlhe_state_base::RAISE_A) != nullptr);
    EXPECT_TRUE(state->raise(0.25) == state->get_action_child(nlhe_state_base::RAISE_A));
}
