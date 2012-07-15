#ifdef _MSC_VER
#pragma warning(push, 3)
#endif
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "cfrlib/cfr_solver.h"
#include "cfrlib/kuhn_game.h"
#include "cfrlib/holdem_game.h"
#include "abslib/holdem_abstraction.h"
#include "cfrlib/kuhn_state.h"
#include "cfrlib/holdem_state.h"
#include "cfrlib/nl_holdem_state.h"

int main(int argc, char* argv[])
{
    namespace po = boost::program_options;

    std::string state_file;
    std::string strategy_file;
    std::string game;
    int iterations = 0;
    std::string abstraction;
    int stack_size;

    po::options_description desc("Options");
    desc.add_options()
        ("help", "produce help message")
        ("state-file", po::value<std::string>(&state_file), "state file")
        ("strategy-file", po::value<std::string>(&strategy_file), "strategy file")
        ("game", po::value<std::string>(&game)->default_value("kuhn"), "game type")
        ("iterations", po::value<int>(&iterations)->default_value(1000000), "number of iterations")
        ("abstraction", po::value<std::string>(&abstraction)->default_value("default"), "abstraction type")
        ("stack-size", po::value<int>(&stack_size)->default_value(200), "stack size in small blinds")
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
    std::cout << "Using abstraction: " << abstraction << "\n";

    std::ifstream abs_file(abstraction, std::ios::binary);

    if (game == "kuhn")
    {
        solver.reset(new cfr_solver<kuhn_game, kuhn_state>(kuhn_abstraction(), stack_size));
    }
    else if (game == "holdem")
    {
        solver.reset(new cfr_solver<holdem_game, holdem_state>(abs_file ? holdem_abstraction(std::move(abs_file)) :
            holdem_abstraction(abstraction), stack_size));
    }
    else if (game == "nlhe")
    {
        solver.reset(new cfr_solver<holdem_game, nl_holdem_state>(abs_file ? holdem_abstraction(std::move(abs_file)) :
            holdem_abstraction(abstraction), stack_size));
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
