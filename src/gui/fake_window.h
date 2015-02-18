#pragma once

#pragma warning(push, 1)
#include <QWidget>
#include <boost/noncopyable.hpp>
#pragma warning(pop)

#include "site_settings.h"

class input_manager;
class window_manager;

class fake_window : private boost::noncopyable
{
public:
    fake_window(const site_settings::window_t& window, const site_settings& settings, const window_manager& wm);
    bool is_valid() const;
    bool click_button(input_manager& input, const site_settings::button_t& button, bool double_click = false) const;
    bool click_any_button(input_manager& input, const site_settings::button_range& buttons, bool double_click = false) const;
    bool update();
    const QImage& get_window_image() const;
    const QImage& get_client_image() const;
    bool is_mouse_inside(const input_manager& input, const QRect& rect) const;
    QRect get_scaled_rect(const QRect& r) const;
    bool is_pixel(const site_settings::pixel_t& pixel) const;

private:
    QPoint client_to_screen(const QPoint& p) const;

    QRect window_rect_;
    QRect client_rect_;
    const QRect rect_;
    QRect title_rect_;
    QImage window_image_;
    QImage client_image_;
    const bool icon_;
    const window_manager& window_manager_;
    const QMargins margins_;
    const bool resizable_;
    const site_settings::button_t* title_button_;
};
