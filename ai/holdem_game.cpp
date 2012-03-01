#include "common.h"
#include "holdem_game.h"
#include "abstraction.h"
#include "evaluator.h"

namespace
{
    static const holdem_game::solver_t::bucket_count_t BUCKET_COUNTS = {3, 9, 9, 9};
}

holdem_game::holdem_game()
    : iterations_(0)
{
    solver_.reset(new solver_t(states_, BUCKET_COUNTS));
}

void holdem_game::solve(const int iterations)
{
    assert(states_.size() == 6378);

    std::random_device rd;
    std::uniform_int_distribution<int> distribution(0, 2);
    const int num_cards_dealt = 9; // 4 hole, 5 board
    const evaluator e;
    const abstraction a(BUCKET_COUNTS);

    std::array<int, 52> deck;

    for (int i = 0; i < deck.size(); ++i)
        deck[i] = i;

    const double start_time = omp_get_wtime();
    double time = start_time;
    int iteration = iterations_;

#pragma omp parallel
    {
        std::mt19937 engine(rd());

#pragma omp for firstprivate(deck, time)
        for (int i = 0; i < iterations; ++i)
        {
            partial_shuffle(deck, num_cards_dealt, engine);

            std::array<std::array<int, 7>, 2> hand;
            hand[0][0] = deck[deck.size() - 1];
            hand[0][1] = deck[deck.size() - 2];
            hand[1][0] = deck[deck.size() - 3];
            hand[1][1] = deck[deck.size() - 4];
            hand[0][2] = hand[1][2] = deck[deck.size() - 5];
            hand[0][3] = hand[1][3] = deck[deck.size() - 6];
            hand[0][4] = hand[1][4] = deck[deck.size() - 7];
            hand[0][5] = hand[1][5] = deck[deck.size() - 8];
            hand[0][6] = hand[1][6] = deck[deck.size() - 9];
       
            const int val1 = e.get_hand_value(hand[0][0], hand[0][1], hand[0][2], hand[0][3], hand[0][4], hand[0][5], hand[0][6]);
            const int val2 = e.get_hand_value(hand[1][0], hand[1][1], hand[1][2], hand[1][3], hand[1][4], hand[1][5], hand[1][6]);
            const int result = val1 > val2 ? 1 : (val1 < val2 ? -1 : 0);

            solver_t::bucket_t buckets;

            for (int k = 0; k < 2; ++k)
            {
                buckets[k][holdem_state::PREFLOP] = a.get_preflop_bucket(hand[k][0], hand[k][1]);
                buckets[k][holdem_state::FLOP] = a.get_flop_bucket(hand[k][0], hand[k][1], hand[k][2], hand[k][3], hand[k][4]);
                buckets[k][holdem_state::TURN] = a.get_turn_bucket(hand[k][0], hand[k][1], hand[k][2], hand[k][3], hand[k][4], hand[k][5]);
                buckets[k][holdem_state::RIVER] = a.get_river_bucket(hand[k][0], hand[k][1], hand[k][2], hand[k][3], hand[k][4], hand[k][5], hand[k][6]);
            }

            std::array<double, 2> reach = {1.0, 1.0};
            solver_->update(*states_[0], buckets, reach, result);

#pragma omp atomic
            ++iteration;

            const double t = omp_get_wtime();

            if (iteration == iterations_ + iterations || (omp_get_thread_num() == 0 && i % 1000 == 0 && t - time >= 1))
            {
                std::array<double, 2> acfr;
                acfr[0] = solver_->get_accumulated_regret(0) / iteration;
                acfr[1] = solver_->get_accumulated_regret(1) / iteration;
                std::cout
                    << "iteration: " << iteration << " (" << (iteration - iterations_) / (t - start_time) << " i/s)"
                    << " regret: " << acfr[0] << ", " << acfr[1] << "\n";
                time = t;
            }
        }
    }

    iterations_ = iteration;
}

std::ostream& holdem_game::print(std::ostream& os) const
{
    for (int i = 0; i < states_.size(); ++i)
    {
        if (states_[i]->is_terminal())
            continue;

        std::string line;

        for (int j = 0; j < solver_->get_bucket_count(states_[i]->get_round()); ++j)
        {
            os << i << "," << j;

            std::array<double, holdem_state::ACTIONS> p;
            solver_->get_average_strategy(*states_[i], j, p);

            for (int k = 0; k < p.size(); ++k)
                os << "," << p[k];

            os << "\n";
        }
    }

    return os;
}

void holdem_game::save(std::ostream& os) const
{
    os.write(reinterpret_cast<const char*>(&iterations_), sizeof(int));
    solver_->save(os);
}

void holdem_game::load(std::istream& is)
{
    is.read(reinterpret_cast<char*>(&iterations_), sizeof(int));
    solver_->load(is);
}
