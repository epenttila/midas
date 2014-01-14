#pragma once

#pragma warning(push, 1)
#include <vector>
#include <QRect>
#include <QWidget>
#pragma warning(pop)

#include "site_settings.h"

class input_manager;

class fake_window
{
public:
    fake_window(const site_settings::window_t& window, const site_settings& settings);
    bool is_valid() const;
    bool click_button(input_manager& input, const site_settings::button_t& button, bool double_click = false) const;
    bool click_any_button(input_manager& input, const site_settings::button_range& buttons, bool double_click = false) const;
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
    const site_settings::font_t* title_font_;
    QRect title_rect_;
    QImage window_image_;
    QImage client_image_;
    bool icon_;
};
