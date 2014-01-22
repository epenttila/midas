#include <random>
#include <iostream>
#include <fstream>
#include <numeric>
#include "lutlib/holdem_preflop_lut.h"
#include "lutlib/holdem_flop_lut.h"
#include "evallib/holdem_evaluator.h"
#include "lutlib/holdem_turn_lut.h"
#include "lutlib/holdem_river_lut.h"
#include "util/partial_shuffle.h"
#include "lutlib/holdem_river_ochs_lut.h"

namespace
{
    void test_river(const holdem_river_lut& lut)
    {
        holdem_evaluator e;
        std::array<int, 52> deck;

        std::iota(deck.begin(), deck.end(), 0);

#pragma omp parallel firstprivate(deck)
        {
            std::random_device rd;
            std::mt19937 engine(rd());

#pragma omp for
            for (int i = 0; i < 1000000; ++i)
            {
                partial_shuffle(deck, 7, engine);

                std::array<int, 5> b = {{deck[deck.size() - 1], deck[deck.size() - 2], deck[deck.size() - 3], deck[deck.size() - 4], deck[deck.size() - 5]}};

                const int c0 = deck[deck.size() - 6];
                const int c1 = deck[deck.size() - 7];

                const float real = float(e.enumerate_river(c0, c1, b[0], b[1], b[2], b[3], b[4]));
                const float cached = lut.get(c0, c1, b[0], b[1], b[2], b[3], b[4]);

                if (cached != real)
                {
                    std::cout << "board: " << c0 << " " << c1 << " | " << b[0] << " " << b[1] << " " << b[2] << " " << b[3] << " " << b[4] << " "
                        << "real ehs: " << real << " cached ehs: " << cached << " err: "
                        << std::abs(real - cached) << "\n";
                }

                if (i % 1000 == 0)
                    std::cout << i << "\n";
            }
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 3)
        return 1;

    const bool generate = std::stoi(argv[1]) ? true : false;
    const int round = std::stoi(argv[2]);

    if (generate)
    {
        switch (round)
        {
        case 0:
            {
                holdem_preflop_lut lut;
                lut.save("holdem_preflop_lut.dat");
            }
            break;
        case 1:
            {
                holdem_flop_lut lut;
                lut.save("holdem_flop_lut.dat");
            }
            break;
        case 2:
            {
                holdem_turn_lut lut;
                lut.save("holdem_turn_lut.dat");
            }
            break;
        case 3:
            {
                holdem_river_lut lut;
                lut.save("holdem_river_lut.dat");
            }
            break;
        case 4:
            {
                holdem_river_ochs_lut lut;
                lut.save("holdem_river_ochs_lut.dat");
            }
            break;
        }
    }
    else
    {
        switch (round)
        {
        case 0:
            {
                const holdem_preflop_lut lut("holdem_preflop_lut.dat");
                //test_flop(lut);
            }
            break;
        case 1:
            {
                const holdem_flop_lut lut("holdem_flop_lut.dat");
                //test_flop(lut);
            }
            break;
        case 2:
            {
                const holdem_turn_lut lut("holdem_turn_lut.dat");
                //test_turn(lut);
            }
            break;
        case 3:
            {
                const holdem_river_lut lut("holdem_river_lut.dat");
                test_river(lut);
            }
            break;
        }
    }
        
    return 0;
}
