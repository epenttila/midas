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
#include "gamelib/kuhn_dealer.h"
#include "gamelib/holdem_dealer.h"
#include "abslib/holdem_abstraction.h"
#include "gamelib/kuhn_state.h"
#include "gamelib/flhe_state.h"
#include "gamelib/nlhe_state.h"
#include "cfrlib/strategy.h"
#include "gamelib/leduc_state.h"
#include "abslib/leduc_abstraction.h"
#include "gamelib/leduc_dealer.h"
#include "cfrlib/pure_cfr_solver.h"
#include "util/version.h"

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
            ("debug-file", po::value<std::string>(&debug_file), "debug output file")
            ("threads", po::value<int>(&threads)->default_value(omp_get_max_threads()), "number of threads")
            ("seed", po::value<std::int64_t>(&seed)->default_value(std::random_device()()), "initial random seed")
            ("log-file", po::value<std::string>(&log_file), "log file")
            ("version", "show version")
            ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help"))
        {
            std::cout << desc << "\n";
            return 0;
        }

        if (vm.count("version"))
        {
            std::cout << util::GIT_VERSION << "\n";
            return 0;
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
        BOOST_LOG_TRIVIAL(info) << "Using abstraction: " << abstraction;

        if (game == "kuhn")
        {
            std::unique_ptr<kuhn_state> state(new kuhn_state);
            std::unique_ptr<kuhn_abstraction> abs(new kuhn_abstraction);

            solver.reset(new pure_cfr_solver<kuhn_dealer, kuhn_state>(std::move(state), std::move(abs)));
        }
        else if (game == "leduc")
        {
            std::unique_ptr<leduc_state> state(new leduc_state);
            std::unique_ptr<leduc_abstraction> abs(new leduc_abstraction);

            solver.reset(new pure_cfr_solver<leduc_dealer, leduc_state>(std::move(state), std::move(abs)));
        }
        else if (game == "holdem")
        {
            std::unique_ptr<flhe_state> state(new flhe_state);
            std::unique_ptr<holdem_abstraction> abs(new holdem_abstraction);
            abs->read(abstraction);

            solver.reset(new pure_cfr_solver<holdem_dealer, flhe_state>(std::move(state), std::move(abs)));
        }
        else if (boost::starts_with(game, "nlhe"))
        {
            std::unique_ptr<nlhe_state> state(nlhe_state::create(game));
            std::unique_ptr<holdem_abstraction> abs(new holdem_abstraction);
            abs->read(abstraction);

            solver.reset(new pure_cfr_solver<holdem_dealer, nlhe_state>(std::move(state), std::move(abs)));
        }

        if (!solver)
            throw std::runtime_error("Unknown game");

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

        solver->connect_progressed([&](std::uint64_t i, const solver_base::cfr_t& cfr) {
            using namespace boost::posix_time;
            const auto d = second_clock::universal_time() - start_time;
            const double ips = d.total_milliseconds() > 0 ? i / double(d.total_milliseconds()) * 1000.0 : 0;
            const auto eta = seconds(ips > 0 ? int((iterations - i) / ips) : 0);
            const double pct = double(i) / iterations * 100.0;
            const solver_base::cfr_t acfr = {{cfr[0] / i, cfr[1] / i}};
            BOOST_LOG_TRIVIAL(info) << boost::format("%d/%d (%.1f%%) ips: %.1f elapsed: %s eta: %s cfr: [%f, %f]")
                % i % iterations % pct % ips % to_simple_string(d) % to_simple_string(eta) % acfr[0] % acfr[1];
        });

        BOOST_LOG_TRIVIAL(info) << "Using random seed: " << seed;
        BOOST_LOG_TRIVIAL(info) << "Using threads: " << threads;
        BOOST_LOG_TRIVIAL(info) << "Solving for " << iterations;
        solver->solve(iterations, seed, threads);

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

        BOOST_LOG_TRIVIAL(info) << "Saving strategy to: " << strategy_file;
        solver->save_strategy(strategy_file);

        return 0;
    }
    catch (const std::exception& e)
    {
        BOOST_LOG_TRIVIAL(error) << e.what();
        return 1;
    }
}
