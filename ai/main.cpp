#include "common.h"
#include "cfr_solver.h"
#include "kuhn_game.h"
#include "holdem_game.h"

int main(int argc, char* argv[])
{
    if (argc != 3)
        return 1;

    const std::string name = argv[1];
    const int iterations = std::atoi(argv[2]);
    std::unique_ptr<solver_base> solver;

    if (name == "kuhn")
    {
        const std::array<int, 1> bucket_counts = {{1}};
        solver.reset(new cfr_solver<kuhn_game>(bucket_counts));
    }
    else if (name == "holdem")
    {
        const std::array<int, holdem_state::ROUNDS> bucket_counts = {{3, 9, 9, 9}};
        solver.reset(new cfr_solver<holdem_game>(bucket_counts));
    }

    {
        std::ifstream f("state.dat");

        if (f)
            solver->load_state(f);
    }

    solver->solve(iterations);

    {
        std::ofstream f("strategy.txt");
        f << *solver;
    }

    {
        std::ofstream f("state.dat");
        solver->save_state(f);
    }

    return 0;
}
