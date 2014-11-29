#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "abslib/holdem_abstraction.h"
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

#ifdef _MSC_VER
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

        namespace po = boost::program_options;

        std::string game;
        std::string abstraction;
        int kmeans_max_iterations;
        float kmeans_tolerance;
        int kmeans_runs;
        std::string log_file;

        po::options_description generic_options("Generic options");
        generic_options.add_options()
            ("help", "produce help message")
            ("game", po::value<std::string>(&game)->required(), "game")
            ("abstraction", po::value<std::string>(&abstraction)->required(), "abstraction")
            ("log-file", po::value<std::string>(&log_file), "log file")
            ("version", "show version")
            ;

        po::options_description holdem_options("holdem options");
        holdem_options.add_options()
            ("kmeans-max-iterations", po::value<int>(&kmeans_max_iterations)->default_value(100),
                "maximum amount of iterations")
            ("kmeans-tolerance", po::value<float>(&kmeans_tolerance)->default_value(1e-4f),
                "relative increment in results to achieve convergence")
            ("kmeans-runs", po::value<int>(&kmeans_runs)->default_value(1),
                "number of times to run k-means")
            ;

        po::options_description desc("Options");
        desc.add(generic_options).add(holdem_options);

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

        BOOST_LOG_TRIVIAL(info) << "abs " << util::GIT_VERSION;

        if (game == "holdem")
        {
            holdem_abstraction abs;
            abs.write(abstraction + ".abs", kmeans_max_iterations, kmeans_tolerance, kmeans_runs);
        }
        else
        {
            throw std::runtime_error("Invalid game");
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        BOOST_LOG_TRIVIAL(error) << e.what();
        return 1;
    }
}
