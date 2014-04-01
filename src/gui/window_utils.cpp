#include "window_utils.h"

#pragma warning(push, 1)
#include <boost/functional/hash.hpp>
#include <QImage>
#include <QDateTime>
#include <boost/log/trivial.hpp>
#pragma warning(pop)

#include "util/card.h"

namespace window_utils
{

std::uint32_t calculate_mask(const QImage& image, const int x, const int top, const int height, const QRgb& color)
{
    int bitmap = 0;

    for (int y = top; y < top + height; ++y)
    {
        const int bit = image.pixel(x, y) == color;
        bitmap |= bit << (y - top);
    }

    return bitmap;
}

std::uint32_t get_hamming_distance(const std::uint32_t x, const std::uint32_t y)
{
    std::uint32_t dist = 0;
    auto val = x ^ y;

    while (val)
    {
        ++dist; 
        val &= val - 1;
    }
 
    return dist;
}

const site_settings::glyph_t* read_exact_glyph(const QImage& image, const QRect& rect, const QRgb& color,
    const site_settings::font_t& font, double* error)
{
    const site_settings::glyph_t* match = nullptr;
    std::size_t hash = 0;

    for (int i = 0; i < rect.width(); ++i)
    {
        const auto mask = calculate_mask(image, rect.left() + i, rect.top(), rect.height(), color);

        boost::hash_combine(hash, mask);

        const auto it = font.masks.find(hash);

        if (it != font.masks.end())
        {
            match = &it->second;
            *error = 0;
        }
    }

    return match;
}

const site_settings::glyph_t* read_fuzzy_glyph(const QImage& image, const QRect& rect, const QRgb& color,
    const site_settings::font_t& font, double tolerance, double* glyph_error)
{
    std::vector<std::uint32_t> columns;

    for (int i = 0; i < rect.width(); ++i)
    {
        const auto mask = calculate_mask(image, rect.left() + i, rect.top(), rect.height(), color);
        columns.push_back(mask);
    }

    const site_settings::glyph_t* match = nullptr;
    int max_popcnt = 0;

    for (const auto& glyph : font.masks)
    {
        const auto popcnt = glyph.second.popcnt;
        int dist = 0;
        double error = 0;

        for (int i = 0; i < glyph.second.columns.size(); ++i)
        {
            const auto mask = i < columns.size() ? columns[i] : 0;

            dist += get_hamming_distance(mask, glyph.second.columns[i]);
            error = static_cast<double>(dist) / popcnt;
        }

        // minimize error and maximize popcnt
        if (error <= tolerance && (error < *glyph_error || (error == *glyph_error && popcnt > max_popcnt)))
        {
            match = &glyph.second;
            *glyph_error = error;
            max_popcnt = popcnt;
        }
    }

    return match;
}

const site_settings::glyph_t* read_glyph(const QImage& image, const QRect& rect, const QRgb& color,
    const site_settings::font_t& font, double tolerance, double* error)
{
    if (tolerance == 0)
        return read_exact_glyph(image, rect, color, font, error);
    else
        return read_fuzzy_glyph(image, rect, color, font, tolerance, error);
}

std::string read_string_impl(const QImage* image, const QRect& rect, const QRgb& color, const site_settings::font_t& font,
    double tolerance, double* error)
{
    if (!image)
        return "";

    std::string s;

    if (error)
        *error = 0;

    for (int x = rect.left(); x < rect.left() + rect.width();)
    {
        const auto max_width = std::min(static_cast<int>(font.max_width), rect.left() + rect.width() - x);
        const QRect glyph_rect(x, rect.top(), max_width, rect.height());
        double glyph_error = std::numeric_limits<double>::max();
        const auto val = read_glyph(*image, glyph_rect, color, font, tolerance, &glyph_error);

        if (val)
        {
            s += val->ch;
            x += static_cast<int>(val->columns.size());

            if (error)
                *error += glyph_error;
        }
        else
        {
            if (error && calculate_mask(*image, x, rect.top(), rect.height(), color) != 0)
                *error += 1;

            ++x;
        }
    }

    if (error)
    {
        if (s.empty())
            *error = std::numeric_limits<double>::max();
        else
            *error /= s.size();
    }

    return s;
}

double get_color_distance(const QRgb& a, const QRgb& b)
{
    return std::sqrt((qRed(a) - qRed(b)) * (qRed(a) - qRed(b))
        + (qGreen(a) - qGreen(b)) * (qGreen(a) - qGreen(b))
        + (qBlue(a) - qBlue(b)) * (qBlue(a) - qBlue(b)));
}

QRgb get_average_color(const QImage& image, const QRect& rect, const QRgb& background, double tolerance)
{
    if (!rect.isValid())
        return 0;

    int red = 0;
    int green = 0;
    int blue = 0;
    int count = 0;

    for (int y = rect.top(); y < rect.top() + rect.height(); ++y)
    {
        for (int x = rect.left(); x < rect.left() + rect.width(); ++x)
        {
            const auto& val = image.pixel(x, y);

            if (background != 0 && get_color_distance(val, background) <= tolerance)
                continue;

            red += qRed(val);
            green += qGreen(val);
            blue += qBlue(val);
            ++count;
        }
    }

    return qRgb(red / count, green / count, blue / count);
}

bool is_pixel(const QImage* image, const site_settings::pixel_t& pixel)
{
    return image && get_color_distance(get_average_color(*image, pixel.rect, 0, 0), pixel.color) <= pixel.tolerance;
}

bool is_button(const QImage* image, const site_settings::button_t& button)
{
    return is_pixel(image, button.pixel);
}

std::string read_string(const QImage* image, const QRect& rect, const QRgb& color, const site_settings::font_t& font,
    double tolerance, const int max_shift)
{
    std::string best_s;
    double min_error = std::numeric_limits<double>::max();
    int best_shift = -1;

    for (int y = -max_shift; y <= max_shift; ++y)
    {
        double error;
        const auto r = rect.adjusted(0, y, 0, y);
        const auto s = read_string_impl(image, r, color, font, tolerance, &error);

        if (error < min_error || (error == min_error && s.size() > best_s.size()))
        {
            best_s = s;
            min_error = error;
            best_shift = y;
        }
    }

    if (!best_s.empty() && best_shift != 0)
    {
        BOOST_LOG_TRIVIAL(warning) << QString("Read shifted string \"%1\" at (%2,%3,%4,%5) with shift %6")
            .arg(best_s.c_str()).arg(rect.left()).arg(rect.top()).arg(rect.width()).arg(rect.height()).arg(best_shift)
            .toStdString();
    }

    return best_s;
}

}
