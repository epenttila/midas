#pragma once

#pragma warning(push, 1)
#include <array>
#include <vector>
#include <cstdint>
#include <QPoint>
#include <QRgb>
#include <QRect>
#include <unordered_map>
#include <regex>
#include <boost/range.hpp>
#include <memory>
#include <QMargins>
#pragma warning(pop)

class site_settings
{
public:
    typedef std::pair<double, double> interval_t;
    typedef std::vector<double> number_list_t;

    struct pixel_t
    {
        QRect unscaled_rect;
        QRgb color;
        double tolerance;
    };

    struct button_t
    {
        QRect unscaled_rect;
        pixel_t pixel;
    };

    struct glyph_t
    {
        std::vector<std::uint32_t> columns;
        std::string ch;
        int popcnt;
    };

    struct font_t
    {
        std::uint32_t max_width;
        std::unordered_map<std::size_t, glyph_t> masks;
    };

    struct label_t
    {
        std::string font;
        QRect unscaled_rect;
        QRgb color;
        double tolerance;
        std::regex regex;
        int shift;
    };

    struct window_t
    {
        bool icon;
        std::string font;
        QRect rect;
        QMargins margins;
        bool resizable;
    };

    struct popup_t
    {
        std::regex regex;
        button_t button;
    };

    typedef std::unordered_multimap<std::string, std::unique_ptr<window_t>> window_map;
    typedef std::unordered_multimap<std::string, std::unique_ptr<std::regex>> regex_map;
    typedef std::unordered_multimap<std::string, std::unique_ptr<pixel_t>> pixel_map;
    typedef std::unordered_multimap<std::string, std::unique_ptr<button_t>> button_map;
    typedef std::unordered_multimap<std::string, std::unique_ptr<font_t>> font_map;
    typedef std::unordered_multimap<std::string, std::unique_ptr<popup_t>> popup_map;
    typedef std::unordered_multimap<std::string, std::unique_ptr<label_t>> label_map;
    typedef std::unordered_multimap<std::string, std::unique_ptr<interval_t>> interval_map;
    typedef std::unordered_multimap<std::string, std::unique_ptr<double>> number_map;
    typedef std::unordered_multimap<std::string, std::unique_ptr<std::string>> string_map;
    typedef std::unordered_multimap<std::string, std::unique_ptr<number_list_t>> number_list_map;
    typedef boost::iterator_range<window_map::const_iterator> window_range;
    typedef boost::iterator_range<button_map::const_iterator> button_range;
    typedef boost::iterator_range<popup_map::const_iterator> popup_range;

    site_settings();
    void load(const std::string& filename);
    std::string get_filename() const;
    window_range get_windows(const std::string& id) const;
    button_range get_buttons(const std::string& id) const;
    popup_range get_popups(const std::string& id) const;
    const font_t* get_font(const std::string& id) const;
    const label_t* get_label(const std::string& id) const;
    const std::regex* get_regex(const std::string& id) const;
    const pixel_t* get_pixel(const std::string& id) const;
    const window_t* get_window(const std::string& id) const;
    const button_t* get_button(const std::string& id) const;
    const interval_t* get_interval(const std::string& id) const;
    const double* get_number(const std::string& id) const;
    const std::string* get_string(const std::string& id) const;
    const number_list_t* get_number_list(const std::string& id) const;

private:
    std::string filename_;
    window_map windows_;
    regex_map regexes_;
    pixel_map pixels_;
    button_map buttons_;
    font_map fonts_;
    popup_map popups_;
    label_map labels_;
    interval_map intervals_;
    number_map numbers_;
    string_map strings_;
    number_list_map number_lists_;
};
