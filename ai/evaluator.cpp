#include "common.h"
#include "evaluator.h"

const int evaluator::RANKS = 32487834;

evaluator::evaluator()
    : ranks_(new int[RANKS])
{
    std::ifstream f("ranks.dat", std::ios_base::binary);
    f.read(reinterpret_cast<char*>(&ranks_[0]), sizeof(ranks_[0]) * RANKS);
}

int evaluator::get_hand_value(const int c0, const int c1, const int c2, const int c3, const int c4) const
{
    int p = ranks_[53 + c0 + 1];
    p = ranks_[p + c1 + 1];
    p = ranks_[p + c2 + 1];
    p = ranks_[p + c3 + 1];
    p = ranks_[p + c4 + 1];
    return ranks_[p];
}

int evaluator::get_hand_value(const int c0, const int c1, const int c2, const int c3, const int c4, const int c5) const
{
    int p = ranks_[53 + c0 + 1];
    p = ranks_[p + c1 + 1];
    p = ranks_[p + c2 + 1];
    p = ranks_[p + c3 + 1];
    p = ranks_[p + c4 + 1];
    p = ranks_[p + c5 + 1];
    return ranks_[p];
}

int evaluator::get_hand_value(const int c0, const int c1, const int c2, const int c3, const int c4, const int c5,
    const int c6) const
{
    int p = ranks_[53 + c0 + 1];
    p = ranks_[p + c1 + 1];
    p = ranks_[p + c2 + 1];
    p = ranks_[p + c3 + 1];
    p = ranks_[p + c4 + 1];
    p = ranks_[p + c5 + 1];
    return ranks_[p + c6 + 1];
}

double evaluator::enumerate_preflop(const int c0, const int c1) const
{
    std::array<bool, 52> used = {};
    used[c0] = true;
    used[c1] = true;
    int count = 0;
    int win = 0;
    int tie = 0;

//#pragma omp parallel for firstprivate(used) reduction(+:count) reduction(+:win) reduction(+:tie)
    for (int b0 = 0; b0 < 48; ++b0)
    {
        if (used[b0])
            continue;

        used[b0] = true;

        for (int b1 = b0 + 1; b1 < 49; ++b1)
        {
            if (used[b1])
                continue;

            used[b1] = true;

            for (int b2 = b1 + 1; b2 < 50; ++b2)
            {
                if (used[b2])
                    continue;

                used[b2] = true;

                for (int b3 = b2 + 1; b3 < 51; ++b3)
                {
                    if (used[b3])
                        continue;

                    used[b3] = true;

                    for (int b4 = b3 + 1; b4 < 52; ++b4)
                    {
                        if (used[b4])
                            continue;

                        used[b4] = true;

                        for (int o0 = 0; o0 < 51; ++o0)
                        {
                            if (used[o0])
                                continue;

                            used[o0] = true;

                            for (int o1 = o0 + 1; o1 < 52; ++o1)
                            {
                                if (used[o1])
                                    continue;

                                const int i = get_hand_value(c0, c1, b0, b1, b2, b3, b4);
                                const int j = get_hand_value(o0, o1, b0, b1, b2, b3, b4);

                                if (i == j)
                                    ++tie;
                                else if (i > j)
                                    ++win;

                                ++count;
                            }

                            used[o0] = false;
                        }

                        used[b4] = false;
                    }

                    used[b3] = false;
                }

                used[b2] = false;
            }

            used[b1] = false;
        }

        used[b0] = false;
    }

    return (win + 0.5 * tie) / count;
}

template<int public_count>
double evaluator::simulate(const std::array<int, 2>& h1, const std::array<int, public_count>& board, const int iterations) const
{
    std::array<int, 52 - public_count - 2> deck;

    for (int i = 0, j = 0; i < 52; ++i)
    {
        if (i != h1[0] && i != h1[1] && std::find(board.begin(), board.end(), i) == board.end())
            deck[j++] = i;
    }

    int win = 0;
    int tie = 0;

    std::random_device rnd;

//#pragma omp parallel firstprivate(deck) reduction(+ : win) reduction(+ : tie)
    {
        std::mt19937 engine(rnd() + omp_get_thread_num());

//#pragma omp for
        for (int i = 0; i < iterations; ++i)
        {
            partial_shuffle(deck, 7 - public_count, engine);

            std::array<int, 2> h2 = {deck[deck.size() - 1], deck[deck.size() - 2]};
            std::array<int, 5> b;

            std::copy(board.begin(), board.end(), b.begin());
            std::copy(deck.rbegin() + h2.size(), deck.rbegin() + h2.size() + 5 - board.size(), b.begin() + board.size());

            const int val0 = get_hand_value(h1[0], h1[1], b[0], b[1], b[2], b[3], b[4]);
            const int val1 = get_hand_value(h2[0], h2[1], b[0], b[1], b[2], b[3], b[4]);

            if (val0 == val1)
                ++tie;
            else if (val0 > val1)
                ++win;
        }
    }

    return (win + 0.5 * tie) / iterations;
}

double evaluator::simulate_preflop(const int c0, const int c1, const int iterations) const
{
    const std::array<int, 2> hand = {c0, c1};
    return simulate<0>(hand, std::array<int, 0>(), iterations);
}

double evaluator::simulate_flop(const int c0, const int c1, const int b0, const int b1, const int b2, const int iterations) const
{
    const std::array<int, 2> hand = {c0, c1};
    const std::array<int, 3> board = {b0, b1, b2};
    return simulate<3>(hand, board, iterations);
}

double evaluator::simulate_turn(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3, const int iterations) const
{
    const std::array<int, 2> hand = {c0, c1};
    const std::array<int, 4> board = {b0, b1, b2, b3};
    return simulate<4>(hand, board, iterations);
}

double evaluator::simulate_river(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3, const int b4, const int iterations) const
{
    const std::array<int, 2> hand = {c0, c1};
    const std::array<int, 5> board = {b0, b1, b2, b3, b4};
    return simulate<5>(hand, board, iterations);
}

const int evaluator::get_rank(int card)
{
    return card / 4;
}

const int evaluator::get_suit(int card)
{
    return card % 4;
}

const int evaluator::get_hand(int rank, int suit)
{
    return rank * 4 + suit;
}
