#pragma once

#pragma warning(push, 1)
#include <unordered_map>
#include <array>
#include <cstdint>
#include <regex>
#include <QPoint>
#include <QColor>
#include <QRect>
#include <QWidget>
#pragma warning(pop)

class QImage;
class QXmlStreamReader;
class input_manager;

namespace window_utils
{
    struct pixel_data
    {
        QPoint point;
        QRgb color;
    };

    struct button_data
    {
        QRect rect;
        pixel_data pixel;
    };

    struct font_data
    {
        std::unordered_map<std::uint32_t, font_data> children;
        std::unordered_map<std::uint32_t, std::pair<std::string, int>> masks;
    };

    struct label_data
    {
        std::string font;
        QRect rect;
    };

    struct popup_data
    {
        std::regex regex;
        button_data button;
    };

    std::uint32_t calculate_mask(const QImage& image, const int x, const int top, const int height, const QRgb& color);
    std::pair<std::string, int> parse_image_char(const QImage& image, const int x, const int y, const int height,
        const QRgb& color, const font_data& font);
    std::string parse_image_text(const QImage* image, const QRect& rect, const QRgb& color, const font_data& font);
    int parse_image_card(const QImage* image, const QImage* mono, const QRect& rect, const std::array<QRgb, 4>& colors,
        const QRgb& color, const font_data& font);
    double parse_image_bet(const QImage* image, const label_data& text, const font_data& font);
    QRect read_xml_rect(QXmlStreamReader& reader);
    pixel_data read_xml_pixel(QXmlStreamReader& reader);
    bool is_pixel(const QImage* image, const pixel_data& pixel);
    bool is_button(const QImage* image, const button_data& button);
    bool is_any_button(const QImage* image, const std::vector<button_data>& buttons);
    bool click_button(input_manager& input, WId window, const button_data& button, bool double_click = false);
    bool click_any_button(input_manager& input, WId window, const std::vector<button_data>& buttons);
    font_data read_xml_font(QXmlStreamReader& reader);
    label_data read_xml_label(QXmlStreamReader& reader);
    button_data read_xml_button(QXmlStreamReader& reader);
    popup_data read_xml_popup(QXmlStreamReader& reader);
    bool close_popups(input_manager& input, WId window, const std::vector<popup_data>& popups, double wait);
}
