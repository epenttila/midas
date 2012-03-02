#pragma once

class holdem_evaluator
{
public:
    enum { CLUB, DIAMOND, HEART, SPADE };
    typedef std::array<int, 7> hand_t;

    holdem_evaluator();
    int get_hand_value(int c0, int c1, int c2, int c3, int c4) const;
    int get_hand_value(int c0, int c1, int c2, int c3, int c4, int c5) const;
    int get_hand_value(int c0, int c1, int c2, int c3, int c4, int c5, int c6) const;
    static int get_rank(int card);
    static int get_suit(int card);
    static int get_hand(int rank, int suit);
    double enumerate_preflop(int c0, int c1) const;
    double simulate_preflop(int c0, int c1, int iterations) const;
    double simulate_flop(int c0, int c1, int b0, int b1, int b2, int iterations) const;
    double simulate_turn(int c0, int c1, int b0, int b1, int b2, int b3, int iterations) const;
    double simulate_river(int c0, int c1, int b0, int b1, int b2, int b3, int b4, int iterations) const;

    template<int public_count>
    double simulate(const std::array<int, 2>& h1, const std::array<int, public_count>& board, const int iterations) const;

private:
    static const int RANKS;
    std::unique_ptr<int[]> ranks_;
};
