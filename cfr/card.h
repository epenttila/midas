enum { CLUB, DIAMOND, HEART, SPADE };

inline int get_rank(const int card)
{
    return card / 4;
}

inline int get_suit(const int card)
{
    return card % 4;
}

inline int get_card(const int rank, const int suit)
{
    return rank * 4 + suit;
}
