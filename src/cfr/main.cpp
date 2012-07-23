#ifdef _MSC_VER
#pragma warning(push, 3)
#endif
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <boost/format.hpp>
#include <boost/date_time.hpp>
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
#include "cfrlib/strategy.h"

int main(int argc, char* argv[])
{
    try
    {
        namespace po = boost::program_options;

        std::string state_file;
        std::string strategy_file;
        std::string game;
        std::uint64_t iterations;
        std::string abstraction;
        int stack_size;

        po::options_description generic_options("Generic options");
        generic_options.add_options()
            ("help", "produce help message")
            ("state-file", po::value<std::string>(&state_file), "state file")
            ("strategy-file", po::value<std::string>(&strategy_file)->required(), "strategy file")
            ("game", po::value<std::string>(&game)->required(), "game type")
            ("iterations", po::value<std::uint64_t>(&iterations)->required(), "number of iterations")
            ("abstraction", po::value<std::string>(&abstraction)->required(), "abstraction type or file")
            ;

        po::options_description nlhe_options("nlhe options");
        nlhe_options.add_options()
            ("stack-size", po::value<int>(&stack_size), "stack size in small blinds")
            ;

        po::options_description desc("Options");
        desc.add(generic_options).add(nlhe_options);

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.empty())
        {
            std::cerr << desc << "\n";
            return 1;
        }
        else if (vm.count("help"))
        {
            std::cout << desc << "\n";
            return 1;
        }

        po::notify(vm);

        std::unique_ptr<solver_base> solver;

        std::cout << "Creating solver for game: " << game << "\n";
        std::cout << "Using abstraction: " << abstraction << "\n";

        // TODO leduc poker
        if (game == "kuhn")
        {
            solver.reset(new cfr_solver<kuhn_game, kuhn_state>(kuhn_abstraction(), stack_size));
        }
        else if (game == "holdem")
        {
            solver.reset(new cfr_solver<holdem_game, holdem_state>(holdem_abstraction(abstraction), stack_size));
        }
        else if (game == "nlhe")
        {
            std::cout << "Using stack size: " << stack_size << "\n";
            solver.reset(new cfr_solver<holdem_game, nl_holdem_state>(holdem_abstraction(abstraction), stack_size));
        }
        else
        {
            std::cout << "Unknown game\n";
            return 1;
        }

        const auto bucket_counts = solver->get_bucket_counts();
        std::cout << "Buckets per round: ";
        
        for (auto i = bucket_counts.begin(); i != bucket_counts.end(); ++i)
            std::cout << (i != bucket_counts.begin() ? ", " : "") << *i;

        std::cout << "\n";

        const auto state_counts = solver->get_state_counts();
        std::cout << "States per round: ";
        
        for (auto i = state_counts.begin(); i != state_counts.end(); ++i)
            std::cout << (i != state_counts.begin() ? ", " : "") << *i;

        std::cout << "\n";

        std::cout << "Initializing storage: " << solver->get_required_memory() << " bytes\n";
        solver->init_storage();

        /*if (!state_file.empty())
        {
            std::cout << "Loading state from: " << state_file << "\n";
            solver->load_state(state_file);
        }*/

        auto start_time = boost::posix_time::second_clock::universal_time();

        solver->connect_progressed([&](std::uint64_t i) {
            using namespace boost::posix_time;
            const auto d = second_clock::universal_time() - start_time;
            const int ips = d.total_seconds() > 0 ? int(i / d.total_seconds()) : 0;
            const auto eta = seconds(ips > 0 ? int((iterations - i) / ips) : 0);
            const double pct = double(i) / iterations * 100.0;
            std::cout << boost::format("%d/%d (%.1f%%) ips: %d elapsed: %s eta: %s\n")
                % i % iterations % pct % ips % to_simple_string(d) % to_simple_string(eta);
        });

        std::cout << "Solving for " << iterations << " iterations\n";
        solver->solve(iterations);

        std::cout << "Saving strategy to: " << strategy_file << "\n";
        solver->save_strategy(strategy_file);

        /*if (!state_file.empty())
        {
            std::cout << "Saving state to: " << state_file << "\n";
            solver->save_state(state_file);
        }*/

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
        return 1;
    }
}
