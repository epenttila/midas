#include "window_utils.h"

#pragma warning(push, 1)
#include <QXmlStreamReader>
#define NOMINMAX
#include <Windows.h>
#pragma warning(pop)

#include "util/card.h"
#include "input_manager.h"
#include "window_manager.h"

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

    return std::make_pair("", 0);
}

std::string parse_image_text(const QImage* image, const QRect& rect, const QRgb& color, const font_data& font)
{
    if (!image)
        return "";

    std::string s;

    for (int x = rect.left(); x < rect.right(); )
    {
        const auto val = parse_image_char(*image, x, rect.top(), rect.height(), color, font);

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

int parse_image_card(const QImage* image, const QRect& rect, const std::array<QRgb, 4>& colors,
    const font_data& font)
{
    if (!image)
        return -1;

    for (int suit = 0; suit < 4; ++suit)
    {
        const auto s = parse_image_text(image, rect, colors[suit], font);

        if (!s.empty())
            return get_card(std::stoi(s), suit);
    }

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

bool click_button(const QImage* image, input_manager& input, WId window, const button_data& button)
{
    input.move_mouse(window, button.rect.x(), button.rect.y(), button.rect.width(), button.rect.height());
    input.sleep();

    if (!is_button(image, button))
        return false;

    input.left_click();

    return true;
}

bool click_any_button(const QImage* image, input_manager& input, WId window, const std::vector<button_data>& buttons)
{
    for (int i = 0; i < buttons.size(); ++i)
    {
        if (click_button(image, input, window, buttons[i]))
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

    const popup_data popup =
    {
        std::regex(reader.attributes().value("pattern").toUtf8()),
        button
    };

    reader.skipCurrentElement();

    return popup;
}

bool close_popup(input_manager& input, WId window, const popup_data& popup)
{
    if (!IsWindow(window) || !IsWindowVisible(window))
        return false;

    const auto title = window_manager::get_window_text(window);

    if (!std::regex_match(title, popup.regex))
        return false;

    const auto image = window_manager::screenshot(window).toImage();
    window_utils::click_button(&image, input, window, popup.button);
    input.sleep();

    if (IsWindow(window))
        SendMessage(window, WM_CLOSE, 0, 0);

    return true;
}

}
