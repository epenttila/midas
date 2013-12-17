#include "window_utils.h"

#pragma warning(push, 1)
#include <boost/log/trivial.hpp>
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

std::pair<std::string, int> parse_image_char(const QImage& image, const int x, const int y, const int height,
    const QRgb& color, const font_data& font)
{
    const auto mask = calculate_mask(image, x, y, height, color);

    const auto i = font.masks.find(mask);

    if (i != font.masks.end())
        return std::make_pair(i->second.first, x + i->second.second);

    const auto j = font.children.find(mask);

    if (j != font.children.end())
        return parse_image_char(image, x + 1, y, height, color, j->second);

    if (mask == 0)
        return std::make_pair("", 0);

    throw std::runtime_error(QString("Unable to parse non-zero mask (0x%1) at (%2,%3)").arg(mask, 0, 16).arg(x).arg(y)
        .toStdString());
}

std::string parse_image_text(const QImage* image, const QRect& rect, const QRgb& color, const font_data& font)
{
    if (!image)
        return "";

    // adjust y in case poker client moves stuff vertically
    bool found = false;
    int y = rect.top();

    for (; y < rect.top() + rect.height(); ++y)
    {
        for (int x = rect.left(); x < rect.left() + rect.width(); ++x)
        {
            // the topmost row should always contain non-background pixels
            if (image->pixel(x, y) == color)
            {
                found = true;
                break;
            }
        }

        if (found)
            break;
    }

    if (!found)
        return "";

    std::string s;

    for (int x = rect.left(); x < rect.left() + rect.width(); )
    {
        const auto val = parse_image_char(*image, x, y, rect.height(), color, font);

        if (val.second > 0)
        {
            s += val.first;
            x = val.second;
        }
        else
        {
            ++x;
        }
    }

    return s;
}

int parse_image_card(const QImage* image, const QImage* mono, const QRect& rect, const std::array<QRgb, 4>& colors,
    const QRgb& color, const font_data& font)
{
    if (!image)
        return -1;

    int suit = -1;

    for (int y = rect.top(); y < rect.top() + rect.height(); ++y)
    {
        for (int x = rect.left(); x < rect.left() + rect.width(); ++x)
        {
            for (int s = 0; s < colors.size(); ++s)
            {
                if (image->pixel(x, y) == colors[s])
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
    {
        //assert(false);
        return -1; // funky colors
    }

    const auto s = parse_image_text(mono, rect, color, font);

    if (!s.empty())
        return get_card(string_to_rank(s), suit);

    return -1;
}

double parse_image_bet(const QImage* image, const label_data& text, const font_data& font)
{
    if (!image)
        return -1;

    const auto s = parse_image_text(image, text.rect, qRgb(255, 255, 255), font);
    double d = std::atof(s.c_str());
        
    if (!s.empty() && s[s.length() - 1] == 'c')
        d /= 100;

    return d;
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
    return image && image->pixel(pixel.point) == pixel.color;
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
    font_data d;

    while (reader.readNextStartElement())
    {
        if (reader.name() == "entry")
        {
            const std::uint32_t mask = reader.attributes().value("mask").toString().toUInt(0, 16);

            if (reader.attributes().hasAttribute("value"))
            {
                assert(d.masks.find(mask) == d.masks.end());

                d.masks[mask] = std::make_pair(reader.attributes().value("value").toUtf8(),
                    reader.attributes().value("width").toString().toInt());

                reader.skipCurrentElement();
            }
            else
            {
                d.children[mask] = read_xml_font(reader);
            }
        }
    }

    return d;
}

label_data read_xml_label(QXmlStreamReader& reader)
{
    const label_data p =
    {
        reader.attributes().value("font").toUtf8(),
        QRect(reader.attributes().value("x").toString().toInt(),
            reader.attributes().value("y").toString().toInt(),
            reader.attributes().value("width").toString().toInt(),
            reader.attributes().value("height").toString().toInt())
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
