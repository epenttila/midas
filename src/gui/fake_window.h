#pragma once

#pragma warning(push, 1)
#include <vector>
#include <QRect>
#include <QWidget>
#pragma warning(pop)

#include "window_utils.h"

class input_manager;

class fake_window
{
public:
    fake_window(const QRect& rect, bool icon, const window_utils::font_data& title_font);
    bool is_valid() const;
    bool click_button(input_manager& input, const window_utils::button_data& button, bool double_click = false) const;
    bool click_any_button(input_manager& input, const std::vector<window_utils::button_data>& buttons, bool double_click = false) const;
    std::string get_window_text() const;
    bool update(WId wid = -1);
    QImage get_window_image() const;
    QImage get_client_image() const;

private:
    QPoint client_to_screen(const QPoint& p) const;

    WId wid_;
    QRect window_rect_;
    QRect client_rect_;
    QRect rect_;
    window_utils::font_data title_font_;
    QRect title_rect_;
    QImage window_image_;
    QImage client_image_;
    bool icon_;
};
