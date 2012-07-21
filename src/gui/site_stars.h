#pragma once

#pragma warning(push, 3)
#include <array>
#include <QWidget>
#pragma warning(pop)

class site_stars
{
public:
    enum { CALL, RAISE };
    site_stars(WId window);
    bool update();
    int get_action() const;
    std::pair<int, int> get_hole_cards() const;
    void get_board_cards(std::array<int, 5>& board) const;
    bool is_new_hand() const;
    int get_round() const;
    double get_raise_fraction() const;

private:
    int dealer_;
    int player_;
    int round_;
    double big_blind_;
    int action_;
    double stack_bb_;
    bool new_hand_;
    std::array<int, 2> hole_;
    std::array<int, 5> board_;
    double fraction_;
    WId window_;
};
