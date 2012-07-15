#pragma once

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
