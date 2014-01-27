#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <random>
#include <numeric>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "cfrlib/nlhe_strategy.h"

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

        std::string strategy_file;

        po::options_description desc("Options");
        desc.add_options()
            ("help", "produce help message")
            ("strategy-file", po::value<std::string>(&strategy_file)->required(), "strategy file")
            ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help"))
        {
            std::cout << desc << "\n";
            return 1;
        }

        po::notify(vm);

        BOOST_LOG_TRIVIAL(info) << "Purifying " << strategy_file;

        nlhe_strategy strategy(strategy_file, false);
        const auto states = game_state_base::get_state_vector(strategy.get_root_state());

        std::size_t count = 0;

        for (int state_id = 0; state_id < static_cast<int>(states.size()); ++state_id)
        {
            const auto& state = *dynamic_cast<const nlhe_state_base*>(states[state_id]);

            for (int bucket = 0; bucket < strategy.get_abstraction().get_bucket_count(state.get_round()); ++bucket)
            {
                strategy::probability_t* begin = strategy.get_strategy().get_data(state, 0, bucket);
                strategy::probability_t* end = begin + state.get_child_count();
                double max = 0;
                int max_count = 0;

                for (auto it = begin; it != end; ++it)
                {
                    if (*it > max)
                    {
                        max = *it;
                        max_count = 1;
                    }
                    else if (*it == max)
                    {
                        ++max_count;
                    }
                }

                for (auto it = begin; it != end; ++it)
                {
                    if (*it == max)
                        *it = static_cast<strategy::probability_t>(1.0 / max_count);
                    else
                        *it = 0;
                }

#ifndef NDEBUG
                assert(std::abs(std::accumulate(begin, end, 0.0) - 1.0) <= 1e-5);

                double val = 0;

                for (auto it = begin; it != end; ++it)
                {
                    if (val == 0 && *it != val)
                        val = *it;

                    assert(*it == val || *it == 0);
                }
#endif

                ++count;
            }
        }

        BOOST_LOG_TRIVIAL(info) << count << " action tuples modified";

        return 0;
    }
    catch (const std::exception& e)
    {
        BOOST_LOG_TRIVIAL(error) << e.what();
        return 1;
    }
}
