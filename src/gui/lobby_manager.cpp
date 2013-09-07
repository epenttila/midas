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
    , registering_(false)
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
        else if (reader.name() == "popup")
        {
            popups_.push_back(window_utils::read_xml_popup(reader));
        }
        else if (reader.name() == "reg-success-popup")
        {
            reg_success_popups_.push_back(window_utils::read_xml_popup(reader));
        }
        else if (reader.name() == "reg-fail-popup")
        {
            reg_fail_popups_.push_back(window_utils::read_xml_popup(reader));
        }
        else if (reader.name() == "finished-popup")
        {
            finished_popups_.push_back(window_utils::read_xml_popup(reader));
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

    for (auto i = lobby->popups_.begin(); i != lobby->popups_.end(); ++i)
        window_utils::close_popup(lobby->input_manager_, window, *i);

    for (auto i = lobby->reg_success_popups_.begin(); i != lobby->reg_success_popups_.end(); ++i)
    {
        if (window_utils::close_popup(lobby->input_manager_, window, *i))
        {
            if (lobby->registering_)
            {
                lobby->registering_ = false;
                ++lobby->registered_;
            }
        }
    }

    for (auto i = lobby->finished_popups_.begin(); i != lobby->finished_popups_.end(); ++i)
    {
        if (window_utils::close_popup(lobby->input_manager_, window, *i))
            --lobby->registered_;
    }

    for (auto i = lobby->reg_fail_popups_.begin(); i != lobby->reg_fail_popups_.end(); ++i)
    {
        if (window_utils::close_popup(lobby->input_manager_, window, *i))
            lobby->registering_ = false;
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
    if (registering_)
        return;

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

    if (!window_utils::click_any_button(&image, input_manager_, window_, register_buttons_))
        return;

    registering_ = true;
}

int lobby_manager::get_registered_sngs() const
{
    return registered_;
}

void lobby_manager::set_window(WId window)
{
    window_ = window;
}
