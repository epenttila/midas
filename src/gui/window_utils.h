#pragma once

#pragma warning(push, 1)
#include <QRect>
#pragma warning(pop)

#include "site_settings.h"

class QImage;

namespace window_utils
{
    std::string parse_image_text(const QImage* image, const QRect& rect, const QRgb& color,
        const site_settings::font_t& font, double tolerance = 0, int max_shift = 0);
    int parse_image_card(const QImage* image, const QImage* mono, const site_settings& settings, const std::string& id);
    std::string parse_image_label(const QImage* image, const site_settings& settings, const site_settings::label_t& label);
    bool is_pixel(const QImage* image, const site_settings::pixel_t& pixel);
    bool is_button(const QImage* image, const site_settings::button_t& button);
    bool is_any_button(const QImage* image, const site_settings::button_range& buttons);
    QRgb get_average_color(const QImage& image, const QRect& rect, const QRgb& background, double tolerance);
    double datetime_to_secs(const QDateTime& dt);
}
