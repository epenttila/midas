#include "common.h"
#include "cfr_solver.h"
#include "kuhn_game.h"
#include "holdem_game.h"

int main(int argc, char* argv[])
{
    namespace po = boost::program_options;

    std::string state_file;
    std::string strategy_file;
    std::string game;
    int iterations = 0;

    po::options_description desc("Options");
    desc.add_options()
        ("help", "produce help message")
        ("state-file", po::value<std::string>(&state_file), "state file")
        ("strategy-file", po::value<std::string>(&strategy_file), "strategy file")
        ("game", po::value<std::string>(&game)->default_value("kuhn"), "game type")
        ("iterations", po::value<int>(&iterations)->default_value(1000000), "number of iterations")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return 1;
    }

    std::unique_ptr<solver_base> solver;

    std::cout << "Creating solver for game: " << game << "\n";

    if (game == "kuhn")
    {
        const std::array<int, 1> bucket_counts = {{1}};
        solver.reset(new cfr_solver<kuhn_game>(bucket_counts));
    }
    else if (game == "holdem")
    {
        const std::array<int, holdem_state::ROUNDS> bucket_counts = {{3, 9, 9, 9}};
        solver.reset(new cfr_solver<holdem_game>(bucket_counts));
    }
    else
    {
        std::cout << "Unknown game\n";
        return 1;
    }

    if (!state_file.empty())
    {
        std::ifstream f(state_file);

        if (f)
        {
            std::cout << "Loading state from: " << state_file << "\n";
            solver->load_state(f);
        }
    }

    std::cout << "Solving for " << iterations << " iterations\n";
    solver->solve(iterations);

    if (strategy_file.empty())
        strategy_file = game + "-strategy.txt";

    std::cout << "Saving strategy to: " << strategy_file << "\n";
    std::ofstream f(strategy_file);
    f << *solver;

    if (!state_file.empty())
    {
        std::ofstream f(state_file);

        if (f)
        {
            std::cout << "Saving state to: " << state_file << "\n";
            solver->save_state(f);
        }
    }

    return 0;
}
