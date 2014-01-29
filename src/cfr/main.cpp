#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <iostream>
#include <fstream>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/algorithm/string.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "cfrlib/cfr_solver.h"
#include "gamelib/kuhn_game.h"
#include "gamelib/holdem_game.h"
#include "abslib/holdem_abstraction.h"
#include "gamelib/kuhn_state.h"
#include "gamelib/flhe_state.h"
#include "gamelib/nlhe_state.h"
#include "cfrlib/strategy.h"
#include "gamelib/leduc_state.h"
#include "abslib/leduc_abstraction.h"
#include "gamelib/leduc_game.h"
#include "cfrlib/pure_cfr_solver.h"
#include "util/version.h"

namespace
{
    template<class Game, class State>
    std::unique_ptr<solver_base> create_solver(const std::string& /*variant*/, std::unique_ptr<State> state,
        std::unique_ptr<typename Game::abstraction_t> abs)
    {
        return std::unique_ptr<pure_cfr_solver<Game, State>>(new pure_cfr_solver<Game, State>(std::move(state), std::move(abs)));
    }

    std::unique_ptr<solver_base> create_nlhe_solver(const std::string& variant, const std::string& config,
        const std::string& abs_filename)
    {
        std::unique_ptr<nlhe_state> state(nlhe_state::create(config));

        typedef holdem_abstraction abstraction_t;
        std::unique_ptr<abstraction_t> abs(new abstraction_t);
        abs->read(abs_filename);
        return create_solver<holdem_game>(variant, std::move(state), std::move(abs));
    }
}

int main(int argc, char* argv[])
{
    try
    {
        namespace log = boost::log;

        log::add_common_attributes();

        static const char* log_format = "[%TimeStamp%] %Message%";

        log::add_console_log
        (
            std::clog,
            log::keywords::format = log_format
        );

        namespace po = boost::program_options;

        std::string state_file;
        std::string strategy_file;
        std::string game;
        std::uint64_t iterations;
        std::string abstraction;
        std::string variant;
        std::string debug_file;
        int threads;
        std::int64_t seed;
        std::string log_file;

        po::options_description desc("Options");
        desc.add_options()
            ("help", "produce help message")
            ("game", po::value<std::string>(&game)->required(), "game type")
            ("abstraction", po::value<std::string>(&abstraction)->required(), "abstraction type or file")
            ("iterations", po::value<std::uint64_t>(&iterations)->required(), "number of iterations")
            ("strategy-file", po::value<std::string>(&strategy_file)->required(), "strategy file")
            ("state-file", po::value<std::string>(&state_file), "state file")
            ("variant", po::value<std::string>(&variant)->required(), "solver variant")
            ("debug-file", po::value<std::string>(&debug_file), "debug output file")
            ("threads", po::value<int>(&threads)->default_value(omp_get_max_threads()), "number of threads")
            ("seed", po::value<std::int64_t>(&seed)->default_value(std::random_device()()), "initial random seed")
            ("log-file", po::value<std::string>(&log_file), "log file")
            ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help"))
        {
            std::cout << desc << "\n";
            return 1;
        }

        po::notify(vm);

        if (!log_file.empty())
        {
            log::add_file_log
            (
                log::keywords::file_name = log_file,
                log::keywords::auto_flush = true,
                log::keywords::format = log_format
            );
        }

        BOOST_LOG_TRIVIAL(info) << "cfr " << util::GIT_VERSION;

        std::unique_ptr<solver_base> solver;

        BOOST_LOG_TRIVIAL(info) << "Creating solver for game: " << game;
        BOOST_LOG_TRIVIAL(info) << "Using solver variant: " << variant;
        BOOST_LOG_TRIVIAL(info) << "Using abstraction: " << abstraction;

        if (game == "kuhn")
        {
            std::unique_ptr<kuhn_state> state(new kuhn_state());
            std::unique_ptr<kuhn_abstraction> abs(new kuhn_abstraction);

            /*if (variant == "cs")
                solver.reset(new cs_cfr_solver<kuhn_game, kuhn_state>(std::move(state), std::move(abs)));
            else if (variant == "pure")*/
                solver.reset(new pure_cfr_solver<kuhn_game, kuhn_state>(std::move(state), std::move(abs)));
        }
        else if (game == "leduc")
        {
            std::unique_ptr<leduc_state> state(new leduc_state());
            std::unique_ptr<leduc_abstraction> abs(new leduc_abstraction);

            solver = create_solver<leduc_game>(variant, std::move(state), std::move(abs));
        }
        else if (game == "holdem")
        {
            std::unique_ptr<flhe_state> state(new flhe_state());
            std::unique_ptr<holdem_abstraction> abs(new holdem_abstraction);
            abs->read(abstraction);

            solver = create_solver<holdem_game>(variant, std::move(state), std::move(abs));
        }
        else if (boost::starts_with(game, "nlhe"))
        {
            solver = create_nlhe_solver(variant, game, abstraction);
        }

        if (!solver)
        {
            BOOST_LOG_TRIVIAL(error) << "Unknown game";
            return 1;
        }

        std::string s;

        for (auto i : solver->get_bucket_counts())
            s += (s.empty() ? "" : ", " ) + std::to_string(i);

        BOOST_LOG_TRIVIAL(info) << "Buckets per round: " << s;

        s.clear();

        for (auto i : solver->get_state_counts())
            s += (s.empty() ? "" : ", " ) + std::to_string(i);

        BOOST_LOG_TRIVIAL(info) << "States per round: " << s;

        s.clear();

        for (auto i : solver->get_action_counts())
            s += (s.empty() ? "" : ", " ) + std::to_string(i);

        BOOST_LOG_TRIVIAL(info) << "Actions per round: " << s;
        
        BOOST_LOG_TRIVIAL(info) << "Initializing storage: " << solver->get_required_memory() << " bytes";
        solver->init_storage();

        if (!state_file.empty())
        {
            BOOST_LOG_TRIVIAL(info) << "Loading state from: " << state_file;
            solver->load_state(state_file);
        }

        auto start_time = boost::posix_time::second_clock::universal_time();

        solver->connect_progressed([&](std::uint64_t i) {
            using namespace boost::posix_time;
            const auto d = second_clock::universal_time() - start_time;
            const double ips = d.total_milliseconds() > 0 ? i / double(d.total_milliseconds()) * 1000.0 : 0;
            const auto eta = seconds(ips > 0 ? int((iterations - i) / ips) : 0);
            const double pct = double(i) / iterations * 100.0;
            BOOST_LOG_TRIVIAL(info) << boost::format("%d/%d (%.1f%%) ips: %.1f elapsed: %s eta: %s")
                % i % iterations % pct % ips % to_simple_string(d) % to_simple_string(eta);
        });

        BOOST_LOG_TRIVIAL(info) << "Using random seed: " << seed;
        BOOST_LOG_TRIVIAL(info) << "Using threads: " << threads;
        BOOST_LOG_TRIVIAL(info) << "Solving for " << iterations;
        solver->solve(iterations, seed, threads);

        BOOST_LOG_TRIVIAL(info) << "Saving strategy to: " << strategy_file;
        solver->save_strategy(strategy_file);

        if (!state_file.empty())
        {
            BOOST_LOG_TRIVIAL(info) << "Saving state to: " << state_file;
            solver->save_state(state_file);
        }

        if (!debug_file.empty())
        {
            BOOST_LOG_TRIVIAL(info) << "Saving debug output to: " << debug_file;
            std::ofstream f(debug_file);
            f << *solver;
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        BOOST_LOG_TRIVIAL(error) << e.what();
        return 1;
    }
}
