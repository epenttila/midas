#include <random>
#include <iostream>
#include <fstream>
#include <numeric>
#include "evallib/holdem_evaluator.h"
#include "lutlib/holdem_river_lut.h"
#include "util/partial_shuffle.h"
#include "lutlib/holdem_river_ochs_lut.h"

namespace
{
    void test_river(const holdem_river_lut& lut)
    {
        std::unique_ptr<holdem_evaluator> e(new holdem_evaluator);
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

                const float real = float(e->enumerate_river(c0, c1, b[0], b[1], b[2], b[3], b[4]));
                const std::array<int, 7> cards = {{c0, c1, b[0], b[1], b[2], b[3], b[4]}};
                const float cached = lut.get(cards);

                if (cached != real)
                {
                    std::cout << "board: " << c0 << " " << c1 << " | " << b[0] << " " << b[1] << " " << b[2] << " " << b[3] << " " << b[4] << " "
                        << "real ehs: " << real << " cached ehs: " << cached << " err: "
                        << std::abs(real - cached) << "\n";
                }
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
                holdem_river_lut lut;
                lut.save("holdem_river_lut.dat");
            }
            break;
        case 1:
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
                const holdem_river_lut lut("holdem_river_lut.dat");
                test_river(lut);
            }
            break;
        }
    }
        
    return 0;
}
