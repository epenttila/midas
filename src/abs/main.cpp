#ifdef _MSC_VER
#pragma warning(push, 3)
#endif
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <regex>
#include <boost/tokenizer.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "abslib/holdem_abstraction.h"
#include "abslib/holdem_abstraction_v2.h"

namespace
{
    void parse_buckets(const std::string& s, int* hs2, int* pub, bool* forget_hs2, bool* forget_pub)
    {
        boost::tokenizer<boost::char_separator<char>> tok(s, boost::char_separator<char>("x"));

        for (auto j = tok.begin(); j != tok.end(); ++j)
        {
            const bool forget = (j->at(j->size() - 1) == 'i');

            if (j->at(0) == 'p')
            {
                *pub = std::atoi(j->substr(1).c_str());
                *forget_pub = forget;
            }
            else
            {
                *hs2 = std::atoi(j->c_str());
                *forget_hs2 = forget;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    try
    {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

        namespace po = boost::program_options;

        std::string game;
        std::string abstraction;
        int kmeans_max_iterations;

        po::options_description generic_options("Generic options");
        generic_options.add_options()
            ("help", "produce help message")
            ("game", po::value<std::string>(&game), "game")
            ("abstraction", po::value<std::string>(&abstraction), "abstraction")
            ;

        po::options_description holdem_options("holdem options");
        holdem_options.add_options()
            ("kmeans-max-iterations", po::value<int>(&kmeans_max_iterations)->default_value(100), "maximum amount of iterations")
            ;

        po::options_description desc("Options");
        desc.add(generic_options).add(holdem_options);

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.empty() || vm.count("help"))
        {
            std::cout << desc << "\n";
            return 1;
        }

        std::cout << "Generating abstraction: " << abstraction << "\n";

        if (game == "holdem")
        {
            holdem_abstraction::bucket_cfg_type cfgs;

            std::smatch m;
            std::regex r("([a-z0-9]+)\\.([a-z0-9]+)\\.([a-z0-9]+)\\.([a-z0-9]+)");

            if (!std::regex_match(abstraction, m, r))
            {
                std::cout << "Invalid abstraction\n";
                return 1;
            }

            int round = 0;

            for (auto i = m.begin() + 1; i != m.end(); ++i)
            {
                auto& cfg = cfgs[round++];
                parse_buckets(*i, &cfg.hs2, &cfg.pub, &cfg.forget_hs2, &cfg.forget_pub);
            }

            const auto abs = holdem_abstraction(cfgs, kmeans_max_iterations);
            const std::string abstraction_file = abstraction + ".abs";
            std::cout << "Saving abstraction to: " << abstraction_file << "\n";
            std::ofstream f(abstraction_file, std::ios::binary);
            abs.save(f);
        }
        else if (game == "holdem-new")
        {
            std::smatch m;
            std::regex r("(pr|ir)-(\\d+)-(\\d+)-(\\d+)-(\\d+)");

            if (!std::regex_match(abstraction, m, r))
            {
                std::cout << "Invalid abstraction\n";
                return 1;
            }

            const bool imperfect_recall = m[1] == "ir" ? true : false;
            holdem_abstraction_v2::bucket_counts_t cfgs;
            cfgs[holdem_abstraction_v2::PREFLOP] = std::stoi(m[2]);
            cfgs[holdem_abstraction_v2::FLOP] = std::stoi(m[3]);
            cfgs[holdem_abstraction_v2::TURN] = std::stoi(m[4]);
            cfgs[holdem_abstraction_v2::RIVER] = std::stoi(m[5]);

            const auto abs = holdem_abstraction_v2(imperfect_recall, cfgs, kmeans_max_iterations);
            const std::string abstraction_file = abstraction + ".abs";
            std::cout << "Saving abstraction to: " << abstraction_file << "\n";
            std::ofstream f(abstraction_file, std::ios::binary);
            abs.save(f);
        }
        else
        {
            std::cerr << "Invalid game\n";
            return 1;
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
        return 1;
    }
}
