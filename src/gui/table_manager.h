#pragma once

#pragma warning(push, 1)
#include <array>
#include <regex>
#include <memory>
#include <boost/noncopyable.hpp>
#include <QImage>
#pragma warning(pop)

#include "window_utils.h"

class input_manager;
class fake_window;

class table_manager : private boost::noncopyable
{
public:
    enum { FOLD, CALL, RAISE, ALLIN, SITOUT };
    enum { FOLD_BUTTON = 0x1, CALL_BUTTON = 0x2, RAISE_BUTTON = 0x4 };
    enum { PLAYER = 0x1, OPPONENT = 0x2 };

    struct snapshot_t
    {
        snapshot_t() : big_blind(-1), total_pot(-1), buttons(0)
        {
            board.fill(-1);
            hole.fill(-1);
            dealer.fill(false);
            stack.fill(-1);
            bet.fill(-1);
            all_in.fill(false);
            sit_out.fill(false);
        }

        std::array<int, 5> board;
        std::array<int, 2> hole;
        std::array<bool, 2> dealer;
        std::array<double, 2> stack;
        std::array<double, 2> bet;
        double big_blind;
        double total_pot;
        std::array<bool, 2> all_in;
        int buttons;
        std::array<bool, 2> sit_out;
    };

    table_manager(const std::string& filename, input_manager& input_manager);
    snapshot_t update(const fake_window& window);
    void fold(double max_wait) const;
    void call(double max_wait) const;
    void raise(double amount, double fraction, double minbet, double max_wait) const;
    void save_snapshot() const;

private:
    void get_hole_cards(std::array<int, 2>& hole) const;
    void get_board_cards(std::array<int, 5>& board) const;
    int get_dealer_mask() const;
    double get_stack(int position) const;
    double get_bet(int position) const;
    double get_big_blind() const;
    double get_total_pot() const;
    bool is_all_in(int position) const;
    int get_buttons() const;
    bool is_sit_out(int position) const;
    double get_pot() const;
    bool is_dealer(int position) const;

    std::string get_stack_text(int position) const;

    std::unique_ptr<fake_window> window_;
    input_manager& input_;
    std::unique_ptr<QImage> image_;
    std::unique_ptr<QImage> mono_image_;

    std::array<window_utils::pixel_data, 2> dealer_pixels_;
    std::array<window_utils::pixel_data, 2> sit_out_pixels_;
    std::array<window_utils::pixel_data, 2> all_in_pixels_;
    std::array<window_utils::pixel_data, 2> stack_hilight_pixels_;

    std::regex title_bb_regex_;

    std::unordered_map<std::string, window_utils::font_data> fonts_;

    window_utils::label_data total_pot_label_;
    window_utils::label_data pot_label_;
    std::array<window_utils::label_data, 2> bet_labels_;
    std::array<window_utils::label_data, 2> stack_labels_;
    std::array<window_utils::label_data, 2> flash_stack_labels_;
    std::string stack_all_in_text_;

    std::array<QRect, 5> board_card_rects_;
    std::array<QRect, 2> hole_card_rects_;
    std::array<QRgb, 4> suit_colors_;
    QRgb card_color_;
    QRgb card_back_color_;

    std::vector<window_utils::button_data> fold_buttons_;
    std::vector<window_utils::button_data> call_buttons_;
    std::vector<window_utils::button_data> raise_buttons_;
    std::vector<window_utils::button_data> bet_input_buttons_;
    std::unordered_multimap<double, window_utils::button_data> size_buttons_;

    std::array<int, 2> window_size_;
    QString table_pattern_;
};
