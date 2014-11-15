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
#include "util/version.h"

void apply_purification(strategy::probability_t* begin, strategy::probability_t* end)
{
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
    double val = 0;

    for (auto it = begin; it != end; ++it)
    {
        if (val == 0 && *it != val)
            val = *it;

        assert(*it == val || *it == 0);
    }
#endif
}

void apply_thresholding(strategy::probability_t* begin, strategy::probability_t* end, const double threshold)
{
    strategy::probability_t sum = 0;

    for (auto it = begin; it != end; ++it)
    {
        if (*it < threshold)
            *it = 0;
        else
            sum += *it;
    }

    for (auto it = begin; it != end; ++it)
        *it /= sum;
}

void apply_new(const nlhe_state& state, strategy::probability_t* begin, strategy::probability_t* end,
    const double threshold)
{
    int fold_index = -1;
    int call_index = -1;
    strategy::probability_t fold = 0;
    strategy::probability_t call = 0;
    strategy::probability_t bet = 0;

    for (int i = 0; i < state.get_child_count(); ++i)
    {
        switch (state.get_child(i)->get_action())
        {
        case nlhe_state::FOLD:
            fold = *(begin + i);
            fold_index = i;
            break;
        case nlhe_state::CALL:
            call = *(begin + i);
            call_index = i;
            break;
        default:
            bet += *(begin + i);
            break;
        }
    }

    if (fold >= threshold || (fold >= call && fold >= bet))
    {
        std::fill(begin, end, strategy::probability_t());
        *(begin + fold_index) = 1;
        return;
    }

    if (call >= bet)
    {
        std::fill(begin, end, strategy::probability_t());
        *(begin + call_index) = 1;
        return;
    }

    if (fold_index != -1)
        *(begin + fold_index) = 0;

    if (call_index != -1)
        *(begin + call_index) = 0;

    apply_purification(begin, end);
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

        std::string strategy_file;
        double threshold;
        int method;

        po::options_description desc("Options");
        desc.add_options()
            ("help", "produce help message")
            ("strategy-file", po::value<std::string>(&strategy_file)->required(), "strategy file")
            ("version", "show version")
            ("threshold", po::value<double>(&threshold)->default_value(0.0), "threshold parameter")
            ("method", po::value<int>(&method)->default_value(0), "post-processing method")
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
            std::cout << util::GIT_VERSION;
            return 0;
        }

        po::notify(vm);

        BOOST_LOG_TRIVIAL(info) << "purify " << util::GIT_VERSION;
        BOOST_LOG_TRIVIAL(info) << "Purifying " << strategy_file;

        nlhe_strategy strategy(strategy_file, false);
        const auto states = game_state_base::get_state_vector(strategy.get_root_state());

        std::size_t count = 0;

        for (int state_id = 0; state_id < static_cast<int>(states.size()); ++state_id)
        {
            const auto& state = *dynamic_cast<const nlhe_state*>(states[state_id]);

            for (int bucket = 0; bucket < strategy.get_abstraction().get_bucket_count(state.get_round()); ++bucket)
            {
                strategy::probability_t* begin = strategy.get_strategy().get_data(state, 0, bucket);
                strategy::probability_t* end = begin + state.get_child_count();

                if (method == 0)
                    apply_purification(begin, end);
                else if (method == 1)
                    apply_thresholding(begin, end, threshold);
                else if (method == 2)
                    apply_new(state, begin, end, threshold);

                assert(std::abs(std::accumulate(begin, end, 0.0) - 1.0) <= 1e-5);

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
