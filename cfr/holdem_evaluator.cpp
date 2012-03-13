#include "holdem_evaluator.h"
#include <random>
#include "partial_shuffle.h"

namespace
{
    template<int N>
    const result simulate(const holdem_evaluator& e, const int c0, const int c1, const std::array<int, N>& board,
        const int iterations)
    {
        std::array<int, 52 - N - 2> deck;

        for (int i = 0, j = 0; i < 52; ++i)
        {
            if (i != c0 && i != c1 && std::find(board.begin(), board.end(), i) == board.end())
                deck[j++] = i;
        }

        result r = {};
        int boards = 0;

        std::random_device rd;
        std::mt19937 engine(rd());

        for (int i = 0; i < iterations; ++i)
        {
            partial_shuffle(deck, 5 - N, engine);

            std::array<int, 5> b;

            std::copy(board.begin(), board.end(), b.begin());
            std::copy(deck.rbegin(), deck.rbegin() + 5 - board.size(), b.begin() + board.size());

            /// @todo does ehs2 work here?
            const double eq = e.enumerate_river(c0, c1, b[0], b[1], b[2], b[3], b[4]);
            r.ehs += eq;
            r.ehs2 += eq * eq;

            ++boards;
        }

        r.ehs /= boards;
        r.ehs2 /= boards;

        return r;
    }
}

holdem_evaluator::holdem_evaluator()
{
}

int holdem_evaluator::get_hand_value(int c0, int c1, int c2, int c3, int c4) const
{
    return evaluator_.get_hand_value(c0, c1, c2, c3, c4);
}

int holdem_evaluator::get_hand_value(int c0, int c1, int c2, int c3, int c4, int c5) const
{
    return evaluator_.get_hand_value(c0, c1, c2, c3, c4, c5);
}

int holdem_evaluator::get_hand_value(int c0, int c1, int c2, int c3, int c4, int c5, int c6) const
{
    return evaluator_.get_hand_value(c0, c1, c2, c3, c4, c5, c6);
}

const result holdem_evaluator::enumerate_preflop(const int c0, const int c1) const
{
    result r = {};
    int boards = 0;

    for (int b0 = 0; b0 < 48; ++b0)
    {
        if (b0 == c0 || b0 == c1)
            continue;

        for (int b1 = b0 + 1; b1 < 49; ++b1)
        {
            if (b1 == c0 || b1 == c1)
                continue;

            for (int b2 = b1 + 1; b2 < 50; ++b2)
            {
                if (b2 == c0 || b2 == c1)
                    continue;

                for (int b3 = b2 + 1; b3 < 51; ++b3)
                {
                    if (b3 == c0 || b3 == c1)
                        continue;

                    for (int b4 = b3 + 1; b4 < 52; ++b4)
                    {
                        if (b4 == c0 || b4 == c1)
                            continue;

                        const double eq = enumerate_river(c0, c1, b0, b1, b2, b3, b4);
                        r.ehs += eq;
                        r.ehs2 += eq * eq;

                        ++boards;
                    }
                }
            }
        }
    }

    r.ehs /= boards;
    r.ehs2 /= boards;

    return r;
}

const result holdem_evaluator::enumerate_flop(const int c0, const int c1, const int b0, const int b1, const int b2) const
{
    result r = {};
    int boards = 0;

    for (int b3 = 0; b3 < 51; ++b3)
    {
        if (b3 == c0 || b3 == c1 || b3 == b0 || b3 == b1 || b3 == b2)
            continue;

        for (int b4 = b3 + 1; b4 < 52; ++b4)
        {
            if (b4 == c0 || b4 == c1 || b4 == b0 || b4 == b1 || b4 == b2)
                continue;

            const double eq = enumerate_river(c0, c1, b0, b1, b2, b3, b4);
            r.ehs += eq;
            r.ehs2 += eq * eq;

            ++boards;
        }
    }

    r.ehs /= boards;
    r.ehs2 /= boards;

    return r;
}

const result holdem_evaluator::enumerate_turn(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3) const
{
    result r = {};
    int boards = 0;

    for (int b4 = 0; b4 < 52; ++b4)
    {
        if (b4 == c0 || b4 == c1 || b4 == b0 || b4 == b1 || b4 == b2 || b4 == b3)
            continue;

        const double eq = enumerate_river(c0, c1, b0, b1, b2, b3, b4);
        r.ehs += eq;
        r.ehs2 += eq * eq;

        ++boards;
    }

    r.ehs /= boards;
    r.ehs2 /= boards;

    return r;
}

double holdem_evaluator::enumerate_river(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3, const int b4) const
{
    const int v0 = evaluator_.get_hand_value(c0, c1, b0, b1, b2, b3, b4);
    int showdowns = 0;
    int wins = 0;
    int ties = 0;

    for (int o0 = 0; o0 < 51; ++o0)
    {
        if (o0 == c0 || o0 == c1 || o0 == b0 || o0 == b1 || o0 == b2 || o0 == b3 || o0 == b4)
            continue;

        for (int o1 = o0 + 1; o1 < 52; ++o1)
        {
            if (o1 == c0 || o1 == c1 || o1 == b0 || o1 == b1 || o1 == b2 || o1 == b3 || o1 == b4)
                continue;

            const int v1 = evaluator_.get_hand_value(o0, o1, b0, b1, b2, b3, b4);

            if (v0 == v1)
                ++ties;
            else if (v0 > v1)
                ++wins;

            ++showdowns;
        }
    }

    return (wins + 0.5 * ties) / showdowns;
}

const result holdem_evaluator::simulate_preflop(const int c0, const int c1, const int iterations) const
{
    return simulate<0>(*this, c0, c1, std::array<int, 0>(), iterations);
}

const result holdem_evaluator::simulate_flop(const int c0, const int c1, const int b0, const int b1, const int b2, const int iterations) const
{
    const std::array<int, 3> board = {{b0, b1, b2}};
    return simulate<3>(*this, c0, c1, board, iterations);
}

const result holdem_evaluator::simulate_turn(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3, const int iterations) const
{
    const std::array<int, 4> board = {{b0, b1, b2, b3}};
    return simulate<4>(*this, c0, c1, board, iterations);
}

const result holdem_evaluator::simulate_river(const int c0, const int c1, const int b0, const int b1, const int b2, const int b3, const int b4, const int iterations) const
{
    const std::array<int, 5> board = {{b0, b1, b2, b3, b4}};
    return simulate<5>(*this, c0, c1, board, iterations);
}
