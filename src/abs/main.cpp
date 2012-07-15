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

    po::options_description desc("Options");
    desc.add_options()
        ("help", "produce help message")
        ("game", po::value<std::string>(&game), "game")
        ("abstraction", po::value<std::string>(&abstraction), "abstraction")
        ;

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
        const auto abs = holdem_abstraction(abstraction);
        const std::string abstraction_file = abstraction + ".abs";
        std::cout << "Saving abstraction to: " << abstraction_file << "\n";
        abs.save(std::ofstream(abstraction_file, std::ios::binary));
    }

    return 0;
}
