#pragma once

#include <cstdint>
#include <string>

typedef std::uint8_t card_t;

enum { CLUB, DIAMOND, HEART, SPADE, SUITS };

static const int RANKS = 13;
static const int CARDS = 52;

inline int get_rank(const int card)
{
    return card >> 2;
}

inline int get_suit(const int card)
{
    return card & 3;
}

inline int get_card(const int rank, const int suit)
{
    return rank << 2 | suit;
}

inline const std::string get_card_string(const int card)
{
    if (card < 0 || card >= 52)
        return "?";

    std::string s(2, 0);

    switch (get_rank(card))
    {
    case 0: s[0] = '2'; break;
    case 1: s[0] = '3'; break;
    case 2: s[0] = '4'; break;
    case 3: s[0] = '5'; break;
    case 4: s[0] = '6'; break;
    case 5: s[0] = '7'; break;
    case 6: s[0] = '8'; break;
    case 7: s[0] = '9'; break;
    case 8: s[0] = 'T'; break;
    case 9: s[0] = 'J'; break;
    case 10: s[0] = 'Q'; break;
    case 11: s[0] = 'K'; break;
    case 12: s[0] = 'A'; break;
    }

    switch (get_suit(card))
    {
    case 0: s[1] = 'c'; break;
    case 1: s[1] = 'd'; break;
    case 2: s[1] = 'h'; break;
    case 3: s[1] = 's'; break;
    }

    return s;
}

inline int string_to_rank(const std::string& s)
{
    if (s.empty())
        return -1;

    int rank;

    switch (s[0])
    {
    case '2': rank = 0; break;
    case '3': rank = 1; break;
    case '4': rank = 2; break;
    case '5': rank = 3; break;
    case '6': rank = 4; break;
    case '7': rank = 5; break;
    case '8': rank = 6; break;
    case '9': rank = 7; break;
    case 'T': rank = 8; break;
    case 'J': rank = 9; break;
    case 'Q': rank = 10; break;
    case 'K': rank = 11; break;
    case 'A': rank = 12; break;
    default: rank = -1;
    }

    return rank;
}

inline int string_to_card(const std::string& s)
{
    if (s.size() < 2)
        return -1;

    const int rank = string_to_rank(s);

    if (rank == -1)
        return -1;

    int suit;

    switch (s[1])
    {
    case 'c': suit = CLUB; break;
    case 'd': suit = DIAMOND; break;
    case 'h': suit = HEART; break;
    case 's': suit = SPADE; break;
    default: suit = -1;
    }

    if (suit == -1)
        return -1;

    return get_card(rank, suit);
}
