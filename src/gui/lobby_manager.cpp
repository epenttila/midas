#include "lobby_manager.h"

#pragma warning(push, 3)
#include <QXmlStreamReader>
#include <QFile>
#include <CommCtrl.h>
#pragma warning(pop)

#include "window_manager.h"
#include "input_manager.h"
#include "table_manager.h"

lobby_manager::lobby_manager(const std::string& filename, input_manager& input_manager)
    : window_(0)
    , input_manager_(input_manager)
    , registered_(0)
{
    QFile file(filename.c_str());

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        throw std::runtime_error("unable to open xml file");

    QXmlStreamReader reader(&file);

    while (reader.readNextStartElement())
    {
        if (reader.name() == "site")
        {
            // do nothing
        }
        else if (reader.name() == "popup-regex")
        {
            popup_regexes_.push_back(std::regex(reader.attributes().value("pattern").toUtf8()));
            reader.skipCurrentElement();
        }
        else if (reader.name() == "registered-regex")
        {
            registered_regexes_.push_back(std::regex(reader.attributes().value("pattern").toUtf8()));
            reader.skipCurrentElement();
        }
        else if (reader.name() == "unregistered-regex")
        {
            unregistered_regexes_.push_back(std::regex(reader.attributes().value("pattern").toUtf8()));
            reader.skipCurrentElement();
        }
        else if (reader.name() == "confirm-regex")
        {
            const lobby_manager::confirm_data d =
            {
                std::regex(reader.attributes().value("pattern").toUtf8()),
                QRect(reader.attributes().value("x").toString().toInt(),
                    reader.attributes().value("y").toString().toInt(),
                    reader.attributes().value("width").toString().toInt(),
                    reader.attributes().value("height").toString().toInt())
            };

            confirm_regexes_.push_back(d);
            reader.skipCurrentElement();
        }
        else if (reader.name() == "game-list-button")
        {
            game_list_buttons_.push_back(window_utils::read_xml_button(reader));
        }
        else if (reader.name() == "unregister-button")
        {
            unregister_buttons_.push_back(window_utils::read_xml_button(reader));
        }
        else if (reader.name() == "register-button")
        {
            register_buttons_.push_back(window_utils::read_xml_button(reader));
        }
        else
        {
            reader.skipCurrentElement();
        }
    }
}

void lobby_manager::close_popups()
{
    EnumWindows(callback, reinterpret_cast<LPARAM>(this));
}

BOOL CALLBACK lobby_manager::callback(HWND window, LPARAM lParam)
{
    if (!IsWindowVisible(window))
        return true;

    lobby_manager* lobby = reinterpret_cast<lobby_manager*>(lParam);

    RECT rect;
    GetClientRect(window, &rect);

    const auto title = window_manager::get_window_text(window);

    for (auto i = lobby->popup_regexes_.begin(); i != lobby->popup_regexes_.end(); ++i)
    {
        if (std::regex_match(title, *i))
            SendMessage(window, WM_CLOSE, 0, 0);
    }

    for (auto i = lobby->registered_regexes_.begin(); i != lobby->registered_regexes_.end(); ++i)
    {
        if (std::regex_match(title, *i))
        {
            ++lobby->registered_;
            SendMessage(window, WM_CLOSE, 0, 0);
        }
    }

    for (auto i = lobby->unregistered_regexes_.begin(); i != lobby->unregistered_regexes_.end(); ++i)
    {
        if (std::regex_match(title, *i))
        {
            --lobby->registered_;

            if (lobby->registered_ < 0)
            {
                assert(false);
                lobby->registered_ = 0;
            }

            SendMessage(window, WM_CLOSE, 0, 0);
        }
    }

    for (auto i = lobby->confirm_regexes_.begin(); i != lobby->confirm_regexes_.end(); ++i)
    {
        if (std::regex_match(title, i->regex))
        {
            lobby->input_manager_.move_mouse(window, i->rect.x(), i->rect.y(), i->rect.width(), i->rect.height());
            lobby->input_manager_.sleep();
            lobby->input_manager_.left_click();
            lobby->input_manager_.sleep();

            if (IsWindow(window))
                SendMessage(window, WM_CLOSE, 0, 0);
        }
    }

    // TODO close ie windows? CloseWindow

    return true;
}

bool lobby_manager::is_window() const
{
    return IsWindow(window_) ? true : false;
}

void lobby_manager::register_sng()
{
    if (!IsWindow(window_))
        return;

    if (IsIconic(window_))
    {
        ShowWindowAsync(window_, SW_RESTORE);
        return;
    }

    auto image = window_manager::screenshot(window_).toImage();

    if (!window_utils::click_any_button(&image, input_manager_, window_, game_list_buttons_))
        return;

    image = window_manager::screenshot(window_).toImage();

    if (window_utils::is_any_button(&image, unregister_buttons_))
        return;

    window_utils::click_any_button(&image, input_manager_, window_, register_buttons_);
}

int lobby_manager::get_registered_sngs() const
{
    return registered_;
}

void lobby_manager::set_window(WId window)
{
    window_ = window;
}
