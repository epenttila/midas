#include "site_settings.h"

#pragma warning(push, 1)
#include <boost/log/trivial.hpp>
#include <QFile>
#include <QXmlStreamReader>
#include <QDateTime>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/log/utility/setup/from_settings.hpp>
#include <QColor>
#include <boost/functional/hash.hpp>
#pragma warning(pop)

namespace
{
    site_settings::pixel_t read_xml_pixel(QXmlStreamReader& reader)
    {
        const site_settings::pixel_t p = {
            QRect(reader.attributes().value("x").toInt(),
                reader.attributes().value("y").toInt(),
                std::max(1, reader.attributes().value("width").toInt()),
                std::max(1, reader.attributes().value("height").toInt())),
            QColor(reader.attributes().value("color").toString()).rgb(),
            reader.attributes().value("tolerance").toDouble()
        };

        reader.skipCurrentElement();

        return p;
    }

    site_settings::font_t read_xml_font(QXmlStreamReader& reader)
    {
        site_settings::font_t font;

        font.max_width = reader.attributes().value("max-width").toUInt();

        while (reader.readNextStartElement())
        {
            if (reader.name() == "glyph")
            {
                site_settings::glyph_t glyph = {};

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

    site_settings::label_t read_xml_label(QXmlStreamReader& reader)
    {
        const site_settings::label_t p = {
            reader.attributes().value("font").toUtf8(),
            QRect(reader.attributes().value("x").toInt(),
                reader.attributes().value("y").toInt(),
                reader.attributes().value("width").toInt(),
                reader.attributes().value("height").toInt()),
            QColor(reader.attributes().value("color").toString()).rgb(),
            reader.attributes().value("tolerance").toDouble(),
            std::regex(reader.attributes().value("pattern").toUtf8()),
            reader.attributes().value("shift").toInt()
        };

        reader.skipCurrentElement();

        return p;
    }

    site_settings::button_t read_xml_button(QXmlStreamReader& reader)
    {
        const site_settings::pixel_t pixel = {
            QRect(reader.attributes().value("color-x").toInt(),
                reader.attributes().value("color-y").toInt(),
                std::max(1, reader.attributes().value("color-width").toInt()),
                std::max(1, reader.attributes().value("color-height").toInt())),
            QColor(reader.attributes().value("color").toString()).rgb(),
            reader.attributes().value("tolerance").toDouble()
        };

        const site_settings::button_t button = {
            QRect(reader.attributes().value("x").toInt(),
                reader.attributes().value("y").toInt(),
                reader.attributes().value("width").toInt(),
                reader.attributes().value("height").toInt()),
            pixel
        };

        reader.skipCurrentElement();

        return button;
    }

    site_settings::window_t read_xml_window(QXmlStreamReader& reader)
    {
        const site_settings::window_t window = {
            reader.attributes().value("icon").toInt() ? true : false,
            reader.attributes().value("font").toString().toStdString(),
            QRect(reader.attributes().value("x").toInt(),
                reader.attributes().value("y").toInt(),
                reader.attributes().value("width").toInt(),
                reader.attributes().value("height").toInt()),
            QMargins(reader.attributes().value("margin-left").toInt(),
                reader.attributes().value("margin-top").toInt(),
                reader.attributes().value("margin-right").toInt(),
                reader.attributes().value("margin-bottom").toInt()),
            reader.attributes().value("resizable").toInt() ? true : false,
            reader.attributes().value("title").toString().toStdString(),
            reader.attributes().value("title-text").toString().toStdString(),
        };

        reader.skipCurrentElement();

        return window;
    }
}

site_settings::site_settings()
    : filename_("N/A")
{
}

void site_settings::load(const std::string& filename)
{
    filename_ = filename;

    windows_.clear();
    regexes_.clear();
    pixels_.clear();
    buttons_.clear();
    fonts_.clear();
    labels_.clear();
    intervals_.clear();
    numbers_.clear();
    strings_.clear();
    number_lists_.clear();

    QFile file(filename.c_str());

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        throw std::runtime_error("unable to open xml file");

    QXmlStreamReader reader(&file);

    while (reader.readNextStartElement())
    {
        if (reader.name() == "settings")
        {
            // do nothing
        }
        else if (reader.name() == "interval")
        {
            const auto id = reader.attributes().value("id").toString().toStdString();
            intervals_.insert(std::make_pair(id, std::unique_ptr<interval_t>(new interval_t(
                reader.attributes().value("min").toDouble(),
                reader.attributes().value("max").toDouble()))));
            reader.skipCurrentElement();
        }
        else if (reader.name() == "number")
        {
            const auto id = reader.attributes().value("id").toString().toStdString();
            numbers_.insert(std::make_pair(id, std::unique_ptr<double>(new double(
                reader.attributes().value("value").toDouble()))));
            reader.skipCurrentElement();
        }
        else if (reader.name() == "string")
        {
            const auto id = reader.attributes().value("id").toString().toStdString();
            strings_.insert(std::make_pair(id, std::unique_ptr<std::string>(new std::string(
                reader.attributes().value("value").toUtf8()))));
            reader.skipCurrentElement();
        }
        else if (reader.name() == "number-list")
        {
            const auto id = reader.attributes().value("id").toString().toStdString();
            std::unique_ptr<number_list_t> list(new number_list_t);

            for (const auto& i : reader.attributes().value("value").toString().split(","))
                list->push_back(i.toDouble());

            number_lists_.insert(std::make_pair(id, std::move(list)));
            reader.skipCurrentElement();
        }
        else if (reader.name() == "button")
        {
            const auto id = reader.attributes().value("id").toString().toStdString();
            buttons_.insert(std::make_pair(id, std::unique_ptr<button_t>(new button_t(read_xml_button(reader)))));
        }
        else if (reader.name() == "window")
        {
            const auto id = reader.attributes().value("id").toString().toStdString();
            windows_.insert(std::make_pair(id, std::unique_ptr<window_t>(new window_t(read_xml_window(reader)))));
        }
        else if (reader.name() == "pixel")
        {
            const auto id = reader.attributes().value("id").toString().toStdString();
            pixels_.insert(std::make_pair(id, std::unique_ptr<pixel_t>(new pixel_t(read_xml_pixel(reader)))));
        }
        else if (reader.name() == "regex")
        {
            const auto id = reader.attributes().value("id").toString().toStdString();
            regexes_.insert(std::make_pair(id, std::unique_ptr<std::regex>(
                new std::regex(reader.attributes().value("value").toUtf8()))));
            reader.skipCurrentElement();
        }
        else if (reader.name() == "font")
        {
            const auto id = reader.attributes().value("id").toString().toStdString();
            fonts_.insert(std::make_pair(id, std::unique_ptr<font_t>(new font_t(read_xml_font(reader)))));
        }
        else if (reader.name() == "label")
        {
            const auto id = reader.attributes().value("id").toString().toStdString();
            labels_.insert(std::make_pair(id, std::unique_ptr<label_t>(new label_t(read_xml_label(reader)))));
        }
        else
        {
            reader.skipCurrentElement();
        }
    }

    try
    {
        boost::log::core::get()->remove_all_sinks();

        boost::property_tree::ptree ptree;
        boost::property_tree::read_xml(filename, ptree);
        boost::log::init_from_settings(boost::log::settings(ptree.get_child("settings.log")));
    }
    catch (const std::exception& e)
    {
        BOOST_LOG_TRIVIAL(error) << e.what();
    }
}

std::string site_settings::get_filename() const
{
    return filename_;
}

site_settings::window_range site_settings::get_windows(const std::string& id)
    const
{
    return windows_.equal_range(id);
}

site_settings::button_range site_settings::get_buttons(const std::string& id) const
{
    return buttons_.equal_range(id);
}

const site_settings::font_t* site_settings::get_font(const std::string& id) const
{
    const auto& i = fonts_.equal_range(id);
    return i.first != i.second ? i.first->second.get() : nullptr;
}

const site_settings::label_t* site_settings::get_label(const std::string& id) const
{
    const auto& i = labels_.equal_range(id);
    return i.first != i.second ? i.first->second.get() : nullptr;
}

const std::regex* site_settings::get_regex(const std::string& id) const
{
    const auto& i = regexes_.equal_range(id);
    return i.first != i.second ? i.first->second.get() : nullptr;
}

const site_settings::pixel_t* site_settings::get_pixel(const std::string& id) const
{
    const auto& i = pixels_.equal_range(id);
    return i.first != i.second ? i.first->second.get() : nullptr;
}

const site_settings::button_t* site_settings::get_button(const std::string& id) const
{
    const auto& i = buttons_.equal_range(id);
    return i.first != i.second ? i.first->second.get() : nullptr;
}

const site_settings::window_t* site_settings::get_window(const std::string& id) const
{
    const auto& i = windows_.equal_range(id);
    return i.first != i.second ? i.first->second.get() : nullptr;
}

const double* site_settings::get_number(const std::string& id) const
{
    const auto& i = numbers_.equal_range(id);
    return i.first != i.second ? i.first->second.get() : nullptr;
}

const site_settings::interval_t* site_settings::get_interval(const std::string& id) const
{
    const auto& i = intervals_.equal_range(id);
    return i.first != i.second ? i.first->second.get() : nullptr;
}

site_settings::interval_t site_settings::get_interval(const std::string& id,
    const site_settings::interval_t& default) const
{
    if (const auto p = get_interval(id))
        return *p;
    else
        return default;
}

const std::string* site_settings::get_string(const std::string& id) const
{
    const auto& i = strings_.equal_range(id);
    return i.first != i.second ? i.first->second.get() : nullptr;
}

std::string site_settings::get_string(const std::string& id, const std::string& default) const
{
    if (const auto p = get_string(id))
        return *p;
    else
        return default;
}

const site_settings::number_list_t* site_settings::get_number_list(const std::string& id) const
{
    const auto& i = number_lists_.equal_range(id);
    return i.first != i.second ? i.first->second.get() : nullptr;
}

site_settings::string_range site_settings::get_strings(const std::string& id) const
{
    return strings_.equal_range(id);
}
