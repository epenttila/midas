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

        for (int i = state->get_actions() - 1; i >= 0; --i)
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
    std::mt19937 engine;
    //engine.seed(1);
    std::uniform_int_distribution<int> distribution(0, 2);
    auto generator = std::bind(distribution, engine);
    //boost::timer::cpu_timer timer;
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

        std::random_shuffle(deck.begin(), deck.end(), generator);
        cards[0] = deck[0];
        cards[1] = deck[1];
        //print [p_hole, o_hole]
        //print_states(root, 0, [p_hole, o_hole])
        solver_->update(*states_[0], cards, reach);
    }

    //print_states(root, 0, [p_hole, o_hole])
}

void kuhn_game::print_strategy()
{
    for (int i = 0; i < states_.size(); ++i)
    {
        if (states_[i]->is_terminal())
            continue;

        std::string line = (boost::format("id=%s,p=%s") % i % states_[i]->get_player()).str();

        for (int j = 0; j < solver_->get_num_buckets(); ++j)
        {
            std::array<double, 2> p;
            solver_->get_average_strategy(*states_[i], j, &p[0]);
            line += (boost::format(" %f") % p[kuhn_state::BET]).str();
        }

        std::cout << line << "\n";
    }
}
