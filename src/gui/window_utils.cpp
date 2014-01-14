#include "window_utils.h"

#pragma warning(push, 1)
#include <boost/log/trivial.hpp>
#include <boost/functional/hash.hpp>
#include <QXmlStreamReader>
#include <QDateTime>
#include <QTime>
#include <Windows.h>
#pragma warning(pop)

#include "util/card.h"
#include "input_manager.h"

Q_GUI_EXPORT QPixmap qt_pixmapFromWinHBITMAP(HBITMAP, int hbitmapFormat = 0);

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

const glyph_data* parse_exact_glyph(const QImage& image, const QRect& rect, const QRgb& color, const font_data& font)
{
    const glyph_data* match = nullptr;
    std::size_t hash = 0;

    for (int i = 0; i < rect.width(); ++i)
    {
        const auto mask = calculate_mask(image, rect.left() + i, rect.top(), rect.height(), color);

        boost::hash_combine(hash, mask);

        const auto it = font.masks.find(hash);

        if (it != font.masks.end())
            match = &it->second;
    }

    return match;
}

const glyph_data* parse_fuzzy_glyph(const QImage& image, const QRect& rect, const QRgb& color, const font_data& font,
    double tolerance)
{
    std::vector<std::uint32_t> columns;

    for (int i = 0; i < rect.width(); ++i)
    {
        const auto mask = calculate_mask(image, rect.left() + i, rect.top(), rect.height(), color);
        columns.push_back(mask);
    }

    const glyph_data* match = nullptr;
    double min_error = std::numeric_limits<double>::max();
    int max_match_width = 0;

    for (const auto& glyph : font.masks)
    {
        int dist = 0;
        double error = 0;

        for (int i = 0; i < columns.size() && i < glyph.second.columns.size(); ++i)
        {
            dist += get_hamming_distance(columns[i], glyph.second.columns[i]);
            error = static_cast<double>(dist) / glyph.second.popcnt;

            if (error > tolerance || error > min_error)
                break;
        }

        const auto width = static_cast<int>(glyph.second.columns.size());

        if (error <= tolerance && (error < min_error || (error == min_error && width > max_match_width)))
        {
            match = &glyph.second;
            min_error = error;
            max_match_width = width;
        }
    }

    return match;
}

const glyph_data* parse_image_char(const QImage& image, const QRect& rect, const QRgb& color, const font_data& font,
    double tolerance)
{
    if (tolerance == 0)
        return parse_exact_glyph(image, rect, color, font);
    else
        return parse_fuzzy_glyph(image, rect, color, font, tolerance);
}

std::string parse_image_text(const QImage* image, const QRect& rect, const QRgb& color, const font_data& font,
    double tolerance)
{
    if (!image)
        return "";

    std::string s;

    for (int x = rect.left(); x < rect.left() + rect.width();)
    {
        const auto mask = calculate_mask(*image, x, rect.top(), rect.height(), color);

        if (mask == 0)
        {
            ++x;
            continue;
        }

        const auto max_width = std::min(static_cast<int>(font.max_width), rect.left() + rect.width() - x);
        const QRect glyph_rect(x, rect.top(), max_width, rect.height());
        const auto val = parse_image_char(*image, glyph_rect, color, font, tolerance);

        if (val)
        {
            s += val->ch;
            x += static_cast<int>(val->columns.size());
        }
        else
        {
            ++x;
        }
    }

    return s;
}

bool is_similar(const QRgb& a, const QRgb& b)
{
    const auto sim = (qRed(a) - qRed(b)) * (qRed(a) - qRed(b))
        + (qGreen(a) - qGreen(b)) * (qGreen(a) - qGreen(b))
        + (qBlue(a) - qBlue(b)) * (qBlue(a) - qBlue(b));

    return sim < 1000;
}


int parse_image_card(const QImage* image, const QImage* mono, const QRect& rect, const std::array<QRgb, 4>& colors,
    const QRgb& color, const QRgb& back_color, const font_data& font)
{
    if (!image)
        return -1;

    if (mono->pixel(rect.topLeft()) != back_color)
        return -1;

    int suit = -1;

    for (int y = rect.top(); y < rect.top() + rect.height(); ++y)
    {
        for (int x = rect.left(); x < rect.left() + rect.width(); ++x)
        {
            for (int s = 0; s < colors.size(); ++s)
            {
                if (is_similar(image->pixel(x, y), colors[s]))
                {
                    suit = s;
                    break;
                }
            }

            if (suit != -1)
                break;
        }

        if (suit != -1)
            break;
    }

    if (suit == -1)
        throw std::runtime_error("Unknown suit color");

    const auto s = parse_image_text(mono, rect, color, font, 0);

    if (!s.empty())
        return get_card(string_to_rank(s), suit);

    return -1;
}

double parse_image_bet(const QImage* image, const label_data& text, const font_data& font)
{
    if (!image)
        return -1;

    const auto s = parse_image_text(image, text.rect, qRgb(255, 255, 255), font, 0);

    static const std::regex re(".*?(\\d+)");
    std::smatch match;

    if (std::regex_match(s, match, re))
        return std::stof(match[1].str());
  
    return 0;
}

QRect read_xml_rect(QXmlStreamReader& reader)
{
    const QRect r = QRect(reader.attributes().value("x").toString().toInt(),
        reader.attributes().value("y").toString().toInt(),
        reader.attributes().value("width").toString().toInt(),
        reader.attributes().value("height").toString().toInt());

    reader.skipCurrentElement();

    return r;
}

pixel_data read_xml_pixel(QXmlStreamReader& reader)
{
    const pixel_data p =
    {
        QPoint(reader.attributes().value("x").toString().toInt(),
            reader.attributes().value("y").toString().toInt()),
        QColor(reader.attributes().value("color").toString()).rgb()
    };

    reader.skipCurrentElement();

    return p;
}

bool is_pixel(const QImage* image, const pixel_data& pixel)
{
    return image && is_similar(image->pixel(pixel.point), pixel.color);
}

bool is_button(const QImage* image, const button_data& button)
{
    return is_pixel(image, button.pixel);
}

bool is_any_button(const QImage* image, const std::vector<button_data>& buttons)
{
    for (int i = 0; i < buttons.size(); ++i)
    {
        if (is_button(image, buttons[i]))
            return true;
    }

    return false;
}

font_data read_xml_font(QXmlStreamReader& reader)
{
    font_data font;

    font.max_width = reader.attributes().value("max-width").toUInt();

    while (reader.readNextStartElement())
    {
        if (reader.name() == "entry")
        {
            glyph_data glyph = {};

            for (const auto& column : reader.attributes().value("mask").toString().split(','))
            {
                const auto val = column.toUInt(nullptr, 16);
                glyph.columns.push_back(val);
                glyph.popcnt += __popcnt(val);
            }

            const auto hash = boost::hash_range(glyph.columns.begin(), glyph.columns.end());
            glyph.ch = reader.attributes().value("value").toString().toStdString();

            assert(font.masks.find(hash) == font.masks.end());

            font.masks[hash] = glyph;

            reader.skipCurrentElement();
        }
    }

    return font;
}

label_data read_xml_label(QXmlStreamReader& reader)
{
    const label_data p =
    {
        reader.attributes().value("font").toUtf8(),
        QRect(reader.attributes().value("x").toString().toInt(),
            reader.attributes().value("y").toString().toInt(),
            reader.attributes().value("width").toString().toInt(),
            reader.attributes().value("height").toString().toInt()),
        QColor(reader.attributes().value("color").toString()).rgb(),
    };

    reader.skipCurrentElement();

    return p;
}

button_data read_xml_button(QXmlStreamReader& reader)
{
    const pixel_data pixel =
    {
        QPoint(reader.attributes().value("color-x").toString().toInt(),
            reader.attributes().value("color-y").toString().toInt()),
        QColor(reader.attributes().value("color").toString()).rgb()
    };

    const button_data button =
    {
        QRect(reader.attributes().value("x").toString().toInt(),
            reader.attributes().value("y").toString().toInt(),
            reader.attributes().value("width").toString().toInt(),
            reader.attributes().value("height").toString().toInt()),
        pixel
    };

    reader.skipCurrentElement();

    return button;
}

popup_data read_xml_popup(QXmlStreamReader& reader)
{
    popup_data popup = {};

    popup.regex = std::regex(reader.attributes().value("pattern").toUtf8());

    if (reader.attributes().hasAttribute("x"))
    {
        const pixel_data pixel =
        {
            QPoint(reader.attributes().value("color-x").toString().toInt(),
                reader.attributes().value("color-y").toString().toInt()),
            QColor(reader.attributes().value("color").toString()).rgb()
        };

        const button_data button =
        {
            QRect(reader.attributes().value("x").toString().toInt(),
                reader.attributes().value("y").toString().toInt(),
                reader.attributes().value("width").toString().toInt(),
                reader.attributes().value("height").toString().toInt()),
            pixel
        };

        popup.button = button;
    }

    reader.skipCurrentElement();

    return popup;
}

}
