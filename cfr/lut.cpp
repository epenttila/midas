#include <random>
#include <iostream>
#include <fstream>
#include "holdem_preflop_lut.h"
#include "holdem_flop_lut.h"
#include "holdem_evaluator.h"
#include "holdem_turn_lut.h"
#include "holdem_river_lut.h"
#include "partial_shuffle.h"

namespace
{
    void test_river(const holdem_river_lut& lut)
    {
        holdem_evaluator e;
        std::array<int, 52> deck;

        for (int i = 0; i < 52; ++i)
            deck[i] = i;

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

    const bool generate = std::atoi(argv[1]) ? true : false;
    const int round = std::atoi(argv[2]);

    if (generate)
    {
        switch (round)
        {
        case 0:
            {
                holdem_preflop_lut lut;
                std::ofstream f("holdem_preflop_lut.dat", std::ios::binary);
                lut.save(f);
            }
            break;
        case 1:
            {
                holdem_flop_lut lut;
                std::ofstream f("holdem_flop_lut.dat", std::ios::binary);
                //lut.save(f);
            }
            break;
        case 2:
            {
                holdem_turn_lut lut;
                std::ofstream f("holdem_turn_lut.dat", std::ios::binary);
                lut.save(f);
            }
            break;
        case 3:
            {
                holdem_river_lut lut;
                std::ofstream f("holdem_river_lut.dat", std::ios::binary);
                lut.save(f);
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
                std::ifstream f("holdem_preflop_lut.dat", std::ios::binary);
                const holdem_preflop_lut lut(f);
                //test_flop(lut);
            }
            break;
        case 1:
            {
                std::ifstream f("holdem_flop_lut.dat", std::ios::binary);
                const holdem_flop_lut lut(f);
                //test_flop(lut);
            }
            break;
        case 2:
            {
                std::ifstream f("holdem_turn_lut.dat", std::ios::binary);
                const holdem_turn_lut lut(f);
                //test_turn(lut);
            }
            break;
        case 3:
            {
                std::ifstream f("holdem_river_lut.dat", std::ios::binary);
                const holdem_river_lut lut(f);
                test_river(lut);
            }
            break;
        }
    }
        
    return 0;
}
