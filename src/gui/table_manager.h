#pragma once

#pragma warning(push, 1)
//#include <utility>
#include <array>
#include <regex>
#include <boost/noncopyable.hpp>
#include <QImage>
#pragma warning(pop)

#include "window_utils.h"

class input_manager;

class table_manager : private boost::noncopyable
{
public:
    enum { FOLD, CALL, RAISE, ALLIN, SITOUT };
    enum { FOLD_BUTTON = 0x1, CALL_BUTTON = 0x2, RAISE_BUTTON = 0x4 };

    table_manager(const std::string& filename, input_manager& input_manager);
    void update(bool save);
    void set_window(WId window);
    std::pair<int, int> get_hole_cards() const;
    void get_board_cards(std::array<int, 5>& board) const;
    int get_dealer() const;
    void fold() const;
    void call() const;
    void raise(double amount, double fraction) const;
    double get_stack(int position) const;
    double get_bet(int position) const;
    double get_big_blind() const;
    double get_total_pot() const;
    int get_active_player() const;
    bool is_all_in(int position) const;
    int get_buttons() const;
    bool is_sit_out(int position) const;
    void sit_in() const;
    bool is_opponent_allin() const;
    bool is_opponent_sitout() const;
    bool is_window() const;

private:
    WId window_;
    input_manager& input_;
    std::unique_ptr<QImage> image_;
    std::unique_ptr<QImage> mono_image_;

    std::array<window_utils::pixel_data, 2> dealer_pixels_;
    std::array<window_utils::pixel_data, 2> player_pixels_;
    std::array<window_utils::pixel_data, 2> sit_out_pixels_;
    std::array<window_utils::pixel_data, 2> all_in_pixels_;
    std::array<window_utils::pixel_data, 2> stack_hilight_pixels_;

    std::regex title_bb_regex_;

    std::unordered_map<std::string, window_utils::font_data> fonts_;

    window_utils::label_data total_pot_label_;
    std::array<window_utils::label_data, 2> bet_labels_;
    std::array<window_utils::label_data, 2> stack_labels_;
    std::array<window_utils::label_data, 2> flash_stack_labels_;

    std::array<QRect, 5> board_card_rects_;
    std::array<QRect, 2> hole_card_rects_;
    std::array<QRgb, 4> suit_colors_;

    std::vector<window_utils::button_data> fold_buttons_;
    std::vector<window_utils::button_data> call_buttons_;
    std::vector<window_utils::button_data> raise_buttons_;
    window_utils::button_data sit_in_button_;
    window_utils::button_data bet_input_button_;
    std::unordered_map<double, window_utils::button_data> size_buttons_;
};
