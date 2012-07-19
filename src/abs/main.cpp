#ifdef _MSC_VER
#pragma warning(push, 3)
#endif
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "abslib/holdem_abstraction.h"

int main(int argc, char* argv[])
{
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
        ("kmeans-max-iterations", po::value<int>(&kmeans_max_iterations), "maximum amount of iterations")
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
        const auto abs = holdem_abstraction(abstraction, kmeans_max_iterations);
        const std::string abstraction_file = abstraction + ".abs";
        std::cout << "Saving abstraction to: " << abstraction_file << "\n";
        std::ofstream f(abstraction_file, std::ios::binary);
        abs.save(f);
    }

    return 0;
}
