#pragma once

#pragma warning(push, 3)
#include <array>
#include <regex>
#include <boost/noncopyable.hpp>
#include <QWidget>
#pragma warning(pop)

#include "site_base.h"

class input_manager;

class site_888 : public site_base, private boost::noncopyable
{
public:
    site_888(input_manager& input_manager, WId window);
    void update();
    std::pair<int, int> get_hole_cards() const;
    void get_board_cards(std::array<int, 5>& board) const;
    int get_dealer() const;
    void fold() const;
    void call() const;
    void raise(double amount) const;
    double get_stack(int player) const;
    double get_bet(int player) const;
    double get_big_blind() const;
    int get_player() const;
    double get_total_pot() const;
    bool is_opponent_allin() const;
    int get_buttons() const;
    bool is_opponent_sitout() const;

private:
    WId window_;
    input_manager& input_;
    std::unique_ptr<QImage> image_;
    std::unique_ptr<QImage> mono_image_;
};
