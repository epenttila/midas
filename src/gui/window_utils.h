#pragma once

#pragma warning(push, 1)
#include <QRect>
#pragma warning(pop)

#include "site_settings.h"

class QImage;

namespace window_utils
{
    std::string read_string(const QImage* image, const QRect& rect, const QRgb& color,
        const site_settings::font_t& font, double tolerance = 0, int max_shift = 0);
    QRgb get_average_color(const QImage& image, const QRect& rect, const QRgb& background, double tolerance);
    double get_color_distance(const QRgb& a, const QRgb& b);
}
