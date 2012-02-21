#include "common.h"
#include "kuhn_game.h"

kuhn_game::kuhn_game()
{
    generate_states();
}

void kuhn_game::generate_states()
{
    int id = 0;
    std::vector<kuhn_state*> stack(1, new kuhn_state(nullptr, -1, 1));

    while (!stack.empty())
    {
        kuhn_state* state = stack.back();
        stack.pop_back();
        state->set_id(id++);
        states_.push_back(state);

        for (int i = kuhn_state::ACTIONS - 1; i >= 0; --i)
        {
            if (kuhn_state* next = state->act(i))
                stack.push_back(next);
        }
    }
}

void kuhn_game::train(const int iterations)
{
    const int num_buckets = 3; // J, Q, K
    const int num_states = int(states_.size());
    solver_.reset(new solver_t(num_states, num_buckets));
    std::mt19937_64 engine;
    engine.seed(1);
    std::uniform_int_distribution<int> distribution(0, 2);
    auto generator = std::bind(distribution, engine, std::placeholders::_1);
    const int num_shuffle_swaps = 2;
    std::array<int, 2> cards;
    std::array<double, 2> reach = {1.0, 1.0};
    std::array<int, num_buckets> deck;

    for (int i = 0; i < num_buckets; ++i)
        deck[i] = i;

    std::clock_t t = std::clock();

    for (int i = 0, ii = 0; i < iterations; ++i)
    {
        if (i > 0 && i % 1000000 == 0)
        {
            std::clock_t tt = std::clock();
            std::cout << boost::format("%d (%f i/s)\n") % i % ((i - ii) / (double(tt - t) / CLOCKS_PER_SEC));
            t = tt;
            ii = i;
        }

        for (auto i = deck.size() - 1; i >= deck.size() - num_shuffle_swaps; --i)
            std::swap(deck[i], deck[generator(int(i + 1))]);

        cards[0] = deck[deck.size() - 1];
        cards[1] = deck[deck.size() - 2];

        solver_->update(*states_[0], cards, reach, cards[0] > cards[1] ? 1 : -1);
    }
}

void kuhn_game::print_strategy()
{
    for (int i = 0; i < states_.size(); ++i)
    {
        if (states_[i]->is_terminal())
            continue;

        std::string line;

        for (const kuhn_state* s = states_[i]; s != nullptr && s->get_action() != -1; s = s->get_parent())
            line = (s->get_action() == kuhn_state::BET ? 'b' : 'p') + line;

        line += '\t';

        for (int j = 0; j < solver_->get_num_buckets(); ++j)
        {
            std::array<double, 2> p;
            solver_->get_average_strategy(*states_[i], j, p);
            line += (boost::format(" %f") % p[kuhn_state::BET]).str();
        }

        std::cout << line << "\n";
    }
}
