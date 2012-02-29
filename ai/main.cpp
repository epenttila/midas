#include "common.h"
#include "kuhn_game.h"
#include "holdem_game.h"

int main(int argc, char* argv[])
{
    if (argc != 3)
        return 1;

    const std::string name = argv[1];
    const int iterations = std::atoi(argv[2]);
    std::unique_ptr<game_base> game;

    if (name == "kuhn")
        game.reset(new kuhn_game);
    else if (name == "holdem")
        game.reset(new holdem_game);

    {
        std::ifstream f("state.dat");

        if (f)
            game->load(f);
    }

    game->solve(iterations);

    {
        std::ofstream f("log.txt");
        f << *game;
    }

    {
        std::ofstream f("state.dat");
        game->save(f);
    }

    return 0;
}
