#include "common.h"
#include "kuhn_game.h"

int main(int argc, char* argv[])
{
    if (argc != 2)
        return 1;

    kuhn_game game;
    game.train(std::atoi(argv[1]));
    game.print_strategy();

    return 0;
}
