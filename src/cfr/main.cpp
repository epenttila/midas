#ifdef _MSC_VER
#pragma warning(push, 3)
#endif
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <boost/format.hpp>
#include <boost/date_time.hpp>
#include <regex>
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

namespace
{
    template<int BITMASK>
    std::unique_ptr<solver_base> create_nlhe_solver(const int stack_size, std::unique_ptr<holdem_abstraction> abs)
    {
        typedef nl_holdem_state<BITMASK> state_type;
        std::unique_ptr<state_type> state(new state_type(stack_size));
        return std::unique_ptr<solver_base>(new cfr_solver<holdem_game, state_type>(std::move(state), std::move(abs)));
    }
}

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

        po::options_description desc("Options");
        desc.add_options()
            ("help", "produce help message")
            ("game", po::value<std::string>(&game)->required(), "game type")
            ("abstraction", po::value<std::string>(&abstraction)->required(), "abstraction type or file")
            ("iterations", po::value<std::uint64_t>(&iterations)->required(), "number of iterations")
            ("strategy-file", po::value<std::string>(&strategy_file)->required(), "strategy file")
            ("state-file", po::value<std::string>(&state_file), "state file")
            ;

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

        std::regex nlhe_regex("nlhe\\.([a-z]+)\\.([0-9]+)");
        std::smatch match;

        // TODO leduc poker
        if (game == "kuhn")
        {
            std::unique_ptr<kuhn_state> state(new kuhn_state());
            std::unique_ptr<kuhn_abstraction> abs(new kuhn_abstraction);
            solver.reset(new cfr_solver<kuhn_game, kuhn_state>(std::move(state), std::move(abs)));
        }
        else if (game == "holdem")
        {
            std::unique_ptr<holdem_state> state(new holdem_state());
            std::unique_ptr<holdem_abstraction> abs(new holdem_abstraction(abstraction));
            solver.reset(new cfr_solver<holdem_game, holdem_state>(std::move(state), std::move(abs)));
        }
        else if (std::regex_match(game, match, nlhe_regex))
        {
            const std::string actions = match[1].str();
            const int stack_size = boost::lexical_cast<int>(match[2].str());
            std::unique_ptr<holdem_abstraction> abs(new holdem_abstraction(abstraction));

            if (actions == "fca")
                solver = create_nlhe_solver<F_MASK | C_MASK | A_MASK>(stack_size, std::move(abs));
            else if (actions == "fcpa")
                solver = create_nlhe_solver<F_MASK | C_MASK | P_MASK | A_MASK>(stack_size, std::move(abs));
            else if (actions == "fchpa")
                solver = create_nlhe_solver<F_MASK | C_MASK | H_MASK | P_MASK | A_MASK>(stack_size, std::move(abs));
            else if (actions == "fchqpa")
                solver = create_nlhe_solver<F_MASK | C_MASK | H_MASK | Q_MASK | P_MASK | A_MASK>(stack_size, std::move(abs));
            else if (actions == "fchqpwdtea")
                solver = create_nlhe_solver<F_MASK | C_MASK | H_MASK | Q_MASK | P_MASK | W_MASK | D_MASK | T_MASK | E_MASK | A_MASK>(stack_size, std::move(abs));
        }

        if (!solver)
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

        if (!state_file.empty())
        {
            std::cout << "Loading state from: " << state_file << "\n";
            solver->load_state(state_file);
        }

        auto start_time = boost::posix_time::second_clock::universal_time();

        solver->connect_progressed([&](std::uint64_t i) {
            using namespace boost::posix_time;
            const auto d = second_clock::universal_time() - start_time;
            const double ips = d.total_milliseconds() > 0 ? i / double(d.total_milliseconds()) * 1000.0 : 0;
            const auto eta = seconds(ips > 0 ? int((iterations - i) / ips) : 0);
            const double pct = double(i) / iterations * 100.0;
            std::cout << boost::format("%d/%d (%.1f%%) ips: %.1f elapsed: %s eta: %s\n")
                % i % iterations % pct % ips % to_simple_string(d) % to_simple_string(eta);
        });

        std::cout << "Solving for " << iterations << " iterations\n";
        solver->solve(iterations);

        std::cout << "Saving strategy to: " << strategy_file << "\n";
        solver->save_strategy(strategy_file);

        if (!state_file.empty())
        {
            std::cout << "Saving state to: " << state_file << "\n";
            solver->save_state(state_file);
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
        return 1;
    }
}
