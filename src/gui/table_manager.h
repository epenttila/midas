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
    enum { FOLD_BUTTON = 0x1, CALL_BUTTON = 0x2, RAISE_BUTTON = 0x4, INPUT_BUTTON = 0x8 };
    enum { PLAYER = 0x1, OPPONENT = 0x2 };
    enum raise_method { DOUBLE_CLICK_INPUT, CLICK_TABLE, MAX_METHODS };

    struct snapshot_t
    {
        snapshot_t() : total_pot(-1), buttons(0), captcha(false)
        {
            board.fill(-1);
            hole.fill(-1);
            dealer.fill(false);
            stack.fill(-1);
            bet.fill(-1);
            all_in.fill(false);
            sit_out.fill(false);
            highlight.fill(false);
        }

        static std::string to_string(const snapshot_t& snapshot);

        std::array<int, 5> board;
        std::array<int, 2> hole;
        std::array<bool, 2> dealer;
        std::array<double, 2> stack;
        std::array<double, 2> bet;
        double total_pot;
        std::array<bool, 2> all_in;
        int buttons;
        std::array<bool, 2> sit_out;
        std::array<int, 2> highlight;
        bool captcha;
    };

    table_manager(const site_settings& settings, input_manager& input_manager);
    snapshot_t update(const fake_window& window);
    void fold(double max_wait) const;
    void call(double max_wait) const;
    void raise(const std::string& action, double amount, double minbet, double max_wait, raise_method method) const;
    void input_captcha(const std::string& str) const;

private:
    void get_hole_cards(std::array<int, 2>& hole) const;
    void get_board_cards(std::array<int, 5>& board) const;
    int get_dealer_mask() const;
    double get_stack(int position) const;
    double get_bet(int position) const;
    double get_total_pot() const;
    bool is_all_in(int position) const;
    int get_buttons() const;
    bool is_sit_out(int position) const;
    double get_pot() const;
    bool is_dealer(int position) const;
    int is_highlight(int position) const;
    bool is_captcha() const;

    std::string get_stack_text(int position) const;
    std::array<int, 3> get_button_map() const;
    std::string get_action_button(int action) const;

    const fake_window* window_;
    input_manager& input_;
    std::unique_ptr<QImage> image_;
    std::unique_ptr<QImage> mono_image_;

    const site_settings* settings_;
};
