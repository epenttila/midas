#ifdef _MSC_VER
#pragma warning(push, 3)
#endif
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <boost/format.hpp>
#include <boost/date_time.hpp>
#include <regex>
#include <numeric>
#include <boost/filesystem.hpp>
#include <cstdint>
#include <boost/algorithm/clamp.hpp>
#include <boost/timer/timer.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "cfrlib/kuhn_game.h"
#include "cfrlib/holdem_game.h"
#include "abslib/holdem_abstraction.h"
#include "cfrlib/kuhn_state.h"
#include "cfrlib/holdem_state.h"
#include "cfrlib/nl_holdem_state.h"
#include "cfrlib/strategy.h"
#include "cfrlib/leduc_state.h"
#include "abslib/leduc_abstraction.h"
#include "cfrlib/leduc_game.h"
#include "util/holdem_loops.h"
#include "evallib/leduc_evaluator.h"
#include "util/partial_shuffle.h"
#include "util/card.h"
#include "actor_base.h"
#include "always_fold_actor.h"
#include "always_call_actor.h"
#include "always_raise_actor.h"
#include "nlhe_actor.h"
#include "nlhe_game.h"

namespace
{
    std::unique_ptr<actor_base> create_actor(const std::string& str)
    {
        std::unique_ptr<actor_base> actor;

        if (str == "fold")
            actor.reset(new always_fold_actor);
        else if (str == "call")
            actor.reset(new always_call_actor);
        else if (str == "raise")
            actor.reset(new always_raise_actor);
        else
            actor.reset(new nlhe_actor(str));

        return actor;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        namespace po = boost::program_options;

        std::string game;
        std::array<std::string, 2> strategy_file;
        std::size_t iterations;
        int seed = std::random_device()();
        std::string history_file;
        std::string plot_file;

        po::options_description desc("Options");
        desc.add_options()
            ("help", "produce help message")
            ("game", po::value<std::string>(&game)->required(), "game to play")
            ("strategy1-file", po::value<std::string>(&strategy_file[0])->required(), "strategy for player 1")
            ("strategy2-file", po::value<std::string>(&strategy_file[1])->required(), "strategy for player 2")
            ("iterations", po::value<std::size_t>(&iterations)->required(), "number of iterations")
            ("seed", po::value<int>(&seed), "rng seed")
            ("history-file", po::value<std::string>(&history_file), "hand history file name")
            ("plot-file", po::value<std::string>(&plot_file), "plotting data file name")
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

        int bankroll = 0;

        std::ofstream plot_stream;

        if (!plot_file.empty())
            plot_stream.open(plot_file);

        boost::timer::auto_cpu_timer t;

        std::regex nlhe_regex("nlhe\\.([0-9]+)");
        std::smatch match;

        if (std::regex_match(game, match, nlhe_regex))
        {
            const int stack_size = std::atoi(match[1].str().c_str());
            auto a1 = create_actor(strategy_file[0]);
            auto a2 = create_actor(strategy_file[1]);
            nlhe_game g(stack_size, *a1, *a2);
            g.set_dealer(0);
            g.set_seed(seed);

            if (!history_file.empty())
                g.set_log(history_file);

            for (std::int64_t i = 0; i < std::int64_t(iterations); ++i)
            {
                if (i > 0 && i % 1000 == 0)
                    std::cerr << "Game #" << i << ": " << (g.get_bankroll() / 2.0 / i * 1000.0) << "\n";

                if (i == std::int64_t(iterations / 2))
                {
                    std::cerr << "Resetting seed at game #" << i << "\n";
                    g.set_seed(seed);
                    g.set_dealer(1);
                }

                g.play(i);

                plot_stream << g.get_bankroll() << "\n";
            }

            bankroll = g.get_bankroll();
        }
        else
        {
            std::cerr << "Unknown game\n";
            return 1;
        }

        double performance = (bankroll / 2.0 / iterations * 1000.0);
        std::clog << "\"" << strategy_file[0] << "\": " << performance << " mb/h\n";
        std::clog << "\"" << strategy_file[1] << "\": " << -performance << " mb/h\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
