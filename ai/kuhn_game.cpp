#include "common.h"
#include "kuhn_game.h"

void kuhn_game::solve(const int iterations)
{
    const int num_cards = 3;
    const std::array<int, 1> num_buckets = {{num_cards}}; // J, Q, K
    solver_.reset(new solver_t(states_, num_buckets));
    std::mt19937_64 engine;
    engine.seed(1);
    const int num_shuffle_swaps = 2;
    solver_t::bucket_t cards = {{}};
    std::array<double, 2> reach = {{1.0, 1.0}};
    std::array<int, num_cards> deck;

    for (int i = 0; i < num_cards; ++i)
        deck[i] = i;

    std::clock_t t = std::clock();
    std::array<double, 2> acfr;

    for (int i = 0, ii = 0; i < iterations; ++i)
    {
        if (i > 0 && i % 1000000 == 0)
        {
            std::clock_t tt = std::clock();
            acfr[0] = solver_->get_accumulated_regret(0) / i;
            acfr[1] = solver_->get_accumulated_regret(1) / i;
            std::cout << boost::format("%d (%f i/s) regret: (%.10f, %.10f)\n") % i % ((i - ii) / (double(tt - t) / CLOCKS_PER_SEC)) % acfr[0] % acfr[1];
            t = tt;
            ii = i;
        }

        partial_shuffle(deck, num_shuffle_swaps, engine);

        cards[0][0] = deck[deck.size() - 1];
        cards[1][0] = deck[deck.size() - 2];

        solver_->update(*states_[0], cards, reach, cards[0] > cards[1] ? 1 : -1);
    }
}

std::ostream& kuhn_game::print(std::ostream& os) const
{
    for (std::size_t i = 0; i < states_.size(); ++i)
    {
        if (states_[i]->is_terminal())
            continue;

        std::string line;

        for (const kuhn_state* s = states_[i]; s != nullptr && s->get_action() != -1; s = s->get_parent())
            line = (s->get_action() == kuhn_state::BET ? 'b' : 'p') + line;

        line += '\t';

        for (int j = 0; j < solver_->get_bucket_count(states_[i]->get_round()); ++j)
        {
            std::array<double, 2> p;
            solver_->get_average_strategy(*states_[i], j, p);
            line += (boost::format(" %f") % p[kuhn_state::BET]).str();
        }

        os << line << "\n";
    }

    return os;
}

void kuhn_game::save(std::ostream& os) const
{
    solver_->save(os);
}

void kuhn_game::load(std::istream& is)
{
    solver_->load(is);
}
