#include "fake_window.h"

#pragma warning(push, 1)
#include <boost/math/special_functions/round.hpp>
#include <boost/log/trivial.hpp>
#pragma warning(pop)

#include "window_utils.h"
#include "input_manager.h"
#include "window_manager.h"

namespace
{
    static const int TITLE_HEIGHT = 19;
    static const int CORNER_SIZE = 3;

    bool is_top_left_corner(const QImage& image, const int x, const int y)
    {
        return image.pixel(x,     y) == qRgb(212, 208, 200)
            && image.pixel(x + 1, y) == qRgb(212, 208, 200)
            && image.pixel(x + 2, y) == qRgb(212, 208, 200)
            && image.pixel(x,     y + 1) == qRgb(212, 208, 200)
            && image.pixel(x + 1, y + 1) == qRgb(255, 255, 255)
            && image.pixel(x + 2, y + 1) == qRgb(255, 255, 255)
            && image.pixel(x,     y + 2) == qRgb(212, 208, 200)
            && image.pixel(x + 1, y + 2) == qRgb(255, 255, 255)
            && image.pixel(x + 2, y + 2) == qRgb(212, 208, 200);
    }

    bool is_top_right_corner(const QImage& image, const int x, const int y)
    {
        return image.pixel(x, y) == qRgb(212, 208, 200)
            && image.pixel(x + 1, y) == qRgb(212, 208, 200)
            && image.pixel(x + 2, y) == qRgb(64, 64, 64)
            && image.pixel(x, y + 1) == qRgb(255, 255, 255)
            && image.pixel(x + 1, y + 1) == qRgb(128, 128, 128)
            && image.pixel(x + 2, y + 1) == qRgb(64, 64, 64)
            && image.pixel(x, y + 2) == qRgb(212, 208, 200)
            && image.pixel(x + 1, y + 2) == qRgb(128, 128, 128)
            && image.pixel(x + 2, y + 2) == qRgb(64, 64, 64);
    }

    bool is_bottom_right_corner(const QImage& image, const int x, const int y)
    {
        return image.pixel(x,     y) == qRgb(212, 208, 200)
            && image.pixel(x + 1, y) == qRgb(128, 128, 128)
            && image.pixel(x + 2, y) == qRgb(64, 64, 64)
            && image.pixel(x,     y + 1) == qRgb(128, 128, 128)
            && image.pixel(x + 1, y + 1) == qRgb(128, 128, 128)
            && image.pixel(x + 2, y + 1) == qRgb(64, 64, 64)
            && image.pixel(x,     y + 2) == qRgb(64, 64, 64)
            && image.pixel(x + 1, y + 2) == qRgb(64, 64, 64)
            && image.pixel(x + 2, y + 2) == qRgb(64, 64, 64);
    }

    bool is_bottom_left_corner(const QImage& image, const int x, const int y)
    {
        return image.pixel(x, y) == qRgb(212, 208, 200)
            && image.pixel(x + 1, y) == qRgb(255, 255, 255)
            && image.pixel(x + 2, y) == qRgb(212, 208, 200)
            && image.pixel(x, y + 1) == qRgb(212, 208, 200)
            && image.pixel(x + 1, y + 1) == qRgb(128, 128, 128)
            && image.pixel(x + 2, y + 1) == qRgb(128, 128, 128)
            && image.pixel(x, y + 2) == qRgb(64, 64, 64)
            && image.pixel(x + 1, y + 2) == qRgb(64, 64, 64)
            && image.pixel(x + 2, y + 2) == qRgb(64, 64, 64);
    }
        
    void try_top_left(const QImage& image, int* border, int* width, int* height)
    {
        // detect extra border
        if (is_top_left_corner(image, 0, 0))
        {
            if (image.pixel(CORNER_SIZE, CORNER_SIZE) == qRgb(212, 208, 200))
                *border = CORNER_SIZE + 1;
            else
                *border = CORNER_SIZE;
        }
        else
            return;

        // left edge
        for (int y = CORNER_SIZE; y < image.height(); ++y)
        {
            if (is_bottom_left_corner(image, 0, y))
            {
                *height = y + CORNER_SIZE;
                break;
            }

            if (image.pixel(0, y) == qRgb(212, 208, 200)
                && image.pixel(1, y) == qRgb(255, 255, 255)
                && image.pixel(2, y) == qRgb(212, 208, 200))
            {
                continue;
            }
            else
            {
                *height = -1;
                break;
            }
        }

        if (*height == -1)
            return;

        // top edge
        for (int x = CORNER_SIZE; x < image.width(); ++x)
        {
            if (is_top_right_corner(image, x, 0))
            {
                *width = x + CORNER_SIZE;
                break;
            }

            if (image.pixel(x, 0) == qRgb(212, 208, 200)
                && image.pixel(x, 1) == qRgb(255, 255, 255)
                && image.pixel(x, 2) == qRgb(212, 208, 200))
            {
                continue;
            }
            else
            {
                *width = -1;
                break;
            }
        }

        if (*width == -1)
            return;

        // right edge
        for (int y = CORNER_SIZE; y < image.height(); ++y)
        {
            if (is_bottom_right_corner(image, *width - CORNER_SIZE, y))
            {
                if (*height != y + CORNER_SIZE)
                    *height = -1;
                break;
            }

            if (image.pixel(*width - 3, y) == qRgb(212, 208, 200)
                && image.pixel(*width - 2, y) == qRgb(128, 128, 128)
                && image.pixel(*width - 1, y) == qRgb(64, 64, 64))
            {
                continue;
            }
            else
            {
                *height = -1;
                break;
            }
        }

        if (*height == -1)
            return;

        // bottom edge
        for (int x = CORNER_SIZE; x < image.width(); ++x)
        {
            if (is_bottom_right_corner(image, x, *height - CORNER_SIZE))
            {
                if (*width != x + CORNER_SIZE)
                    *width = -1;
                break;
            }

            if (image.pixel(x, *height - 3) == qRgb(212, 208, 200)
                && image.pixel(x, *height - 2) == qRgb(128, 128, 128)
                && image.pixel(x, *height - 1) == qRgb(64, 64, 64))
            {
                continue;
            }
            else
            {
                *width = -1;
                break;
            }
        }

        if (*width == -1)
            return;
    }
}

fake_window::fake_window()
    : window_manager_(nullptr)
    , nonclient_(false)
    , border_color_(0)
{
}

fake_window::fake_window(const site_settings::window_t& window, const window_manager& wm)
    : rect_(window.rect)
    , window_manager_(&wm)
    , nonclient_(window.nonclient)
    , border_color_(window.border_color)
{
}

bool fake_window::update()
{
    window_rect_ = QRect();
    client_rect_ = QRect();
    window_image_ = QImage();
    client_image_ = QImage();

    const auto image = window_manager_ ? window_manager_->get_image().copy(rect_) : QImage();

    if (image.isNull())
        return true; // no capture

    if (nonclient_)
    {
        // top left
        int border = -1;
        int width = -1;
        int height = -1;

        try_top_left(image, &border, &width, &height);

        if (border == -1)
            return true; // no window

        if (width == -1 || height == -1)
        {
            BOOST_LOG_TRIVIAL(warning) << "Unable to determine window size";
            return false;
        }

        window_rect_ = QRect(rect_.left(), rect_.top(), width, height);
        client_rect_ = window_rect_.adjusted(border, border + TITLE_HEIGHT, -border, -border);
    }
    else
    {
        if (image.pixel(0, 0) != border_color_)
            return true;

        window_rect_ = rect_;
        client_rect_ = window_rect_;
    }

    if (window_rect_ != rect_)
    {
        BOOST_LOG_TRIVIAL(warning) << QString("Window has been resized (%1,%2,%3,%4) != (%5,%6,%7,%8)")
            .arg(window_rect_.x())
            .arg(window_rect_.y())
            .arg(window_rect_.width())
            .arg(window_rect_.height())
            .arg(rect_.x())
            .arg(rect_.y())
            .arg(rect_.width())
            .arg(rect_.height()).toStdString();
    }

    window_image_ = image.copy(window_rect_.translated(-window_rect_.topLeft()));
    client_image_ = image.copy(client_rect_.translated(-window_rect_.topLeft()));

    return true;
}

bool fake_window::is_valid() const
{
    if (!window_rect_.isValid() || !client_rect_.isValid() || window_image_.isNull() || client_image_.isNull())
        return false;

    return true;
}

bool fake_window::click_button(input_manager& input, const site_settings::button_t& button, bool double_click) const
{
    // check if button is there
    if (!is_pixel(button.pixel))
        return false;

    const auto& rect = get_scaled_rect(button.unscaled_rect);
    const auto p = client_to_screen(QPoint(rect.x(), rect.y()));
    input.move_click(p.x(), p.y(), rect.width(), rect.height(), double_click);

    return true;
}

bool fake_window::click_any_button(input_manager& input, const site_settings::button_range& buttons, bool double_click) const
{
    for (const auto& i : buttons)
    {
        if (click_button(input, *i.second, double_click))
            return true;
    }

    return false;
}

QPoint fake_window::client_to_screen(const QPoint& point) const
{
    return window_manager_ ? window_manager_->client_to_screen(client_rect_.topLeft() + point) : QPoint();
}

const QImage& fake_window::get_window_image() const
{
    return window_image_;
}

const QImage& fake_window::get_client_image() const
{
    return client_image_;
}

bool fake_window::is_mouse_inside(const input_manager& input, const QRect& rect) const
{
    if (!is_valid())
        return false;

    const QRect r = get_scaled_rect(QRect(client_to_screen(rect.topLeft()), client_to_screen(rect.bottomRight())));

    return input.is_mouse_inside(r.x(), r.y(), r.width(), r.height());
}

QRect fake_window::get_scaled_rect(const QRect& r) const
{
    if (rect_.isNull())
        return r;

    const auto x_scale = static_cast<double>(window_rect_.width()) / rect_.width();
    const auto y_scale = static_cast<double>(window_rect_.height()) / rect_.height();

    return QRect(
        boost::math::iround(r.x() * x_scale),
        boost::math::iround(r.y() * y_scale),
        boost::math::iround(r.width() * x_scale),
        boost::math::iround(r.height() * y_scale));
}

bool fake_window::is_pixel(const site_settings::pixel_t& pixel) const
{
    const auto& rect = get_scaled_rect(pixel.unscaled_rect);
    const auto avg = window_utils::get_average_color(client_image_, rect, 0, 0);

    return !client_image_.isNull() && window_utils::get_color_distance(avg, pixel.color) <= pixel.tolerance;
}
