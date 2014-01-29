#include "gtest/gtest.h"
#include "cfrlib/pure_cfr_solver.h"
#include "cfrlib/kuhn_state.h"
#include "abslib/kuhn_abstraction.h"
#include "cfrlib/kuhn_game.h"

namespace
{
    static const double eps = 0.01;
    enum { JACK, QUEEN, KING };
}

TEST(pure_cfr_solver, kuhn)
{
    std::unique_ptr<kuhn_state> state(new kuhn_state);
    std::unique_ptr<kuhn_abstraction> abs(new kuhn_abstraction);

    pure_cfr_solver<kuhn_game, kuhn_state> solver(std::move(state), std::move(abs));
    solver.init_storage();
    solver.solve(1000000, 0);
    //solver.print(std::cout);

    const auto& root = solver.get_root_state();
    std::array<float, kuhn_state::ACTIONS> data;

    // P1 acts

    auto s = &root;

    solver.get_average_strategy(*s, KING, data.data());

    const auto gamma = data[kuhn_state::BET];
    const auto alpha = gamma / 3.0;
    const auto beta = (1.0 + gamma) / 3.0;
    const auto eta = 1.0 / 3.0;
    const auto xi = 1.0 / 3.0;

    EXPECT_NEAR(1 - gamma, data[kuhn_state::PASS], eps);
    EXPECT_NEAR(    gamma, data[kuhn_state::BET], eps);

    solver.get_average_strategy(*s, JACK, data.data());

    EXPECT_NEAR(1 - alpha, data[kuhn_state::PASS], eps);
    EXPECT_NEAR(    alpha, data[kuhn_state::BET], eps);

    solver.get_average_strategy(*s, QUEEN, data.data());

    EXPECT_NEAR(1.0, data[kuhn_state::PASS], eps);
    EXPECT_NEAR(0.0, data[kuhn_state::BET], eps);

    // P1 pass, P2 acts

    s = root.get_child(kuhn_state::PASS);

    solver.get_average_strategy(*s, JACK, data.data());

    EXPECT_NEAR(1 - xi, data[kuhn_state::PASS], eps);
    EXPECT_NEAR(    xi, data[kuhn_state::BET], eps);

    solver.get_average_strategy(*s, QUEEN, data.data());

    EXPECT_NEAR(1.0, data[kuhn_state::PASS], eps);
    EXPECT_NEAR(0.0, data[kuhn_state::BET], eps);

    solver.get_average_strategy(*s, KING, data.data());

    EXPECT_NEAR(0.0, data[kuhn_state::PASS], eps);
    EXPECT_NEAR(1.0, data[kuhn_state::BET], eps);

    // P1 bet, P2 acts

    s = root.get_child(kuhn_state::BET);

    solver.get_average_strategy(*s, JACK, data.data());

    EXPECT_NEAR(1.0, data[kuhn_state::PASS], eps);
    EXPECT_NEAR(0.0, data[kuhn_state::BET], eps);

    solver.get_average_strategy(*s, QUEEN, data.data());

    EXPECT_NEAR(1 - eta, data[kuhn_state::PASS], eps);
    EXPECT_NEAR(    eta, data[kuhn_state::BET], eps);

    solver.get_average_strategy(*s, KING, data.data());

    EXPECT_NEAR(0.0, data[kuhn_state::PASS], eps);
    EXPECT_NEAR(1.0, data[kuhn_state::BET], eps);

    // P1 pass, P2 bet, P1 acts

    s = root.get_child(kuhn_state::PASS)->get_child(kuhn_state::BET);

    solver.get_average_strategy(*s, JACK, data.data());

    EXPECT_NEAR(1.0, data[kuhn_state::PASS], eps);
    EXPECT_NEAR(0.0, data[kuhn_state::BET], eps);

    solver.get_average_strategy(*s, QUEEN, data.data());

    EXPECT_NEAR(1 - beta, data[kuhn_state::PASS], eps);
    EXPECT_NEAR(    beta, data[kuhn_state::BET], eps);

    solver.get_average_strategy(*s, KING, data.data());

    EXPECT_NEAR(0.0, data[kuhn_state::PASS], eps);
    EXPECT_NEAR(1.0, data[kuhn_state::BET], eps);
}
