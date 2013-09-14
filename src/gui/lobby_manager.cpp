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

BOOL CALLBACK lobby_manager::callback(HWND hwnd, LPARAM lParam)
{
    lobby_manager* lobby = reinterpret_cast<lobby_manager*>(lParam);
    const auto window = reinterpret_cast<WId>(hwnd);

    window_utils::close_popups(lobby->input_manager_, window, lobby->popups_);

    if (window_utils::close_popups(lobby->input_manager_, window, lobby->reg_success_popups_))
    {
        assert(lobby->registering_);
        lobby->registering_ = false;
        ++lobby->registered_;
    }

    if (window_utils::close_popups(lobby->input_manager_, window, lobby->finished_popups_))
        --lobby->registered_;

    if (window_utils::close_popups(lobby->input_manager_, window, lobby->reg_fail_popups_))
    {
        assert(lobby->registering_);
        lobby->registering_ = false;
    }

    // TODO close ie windows? CloseWindow

    return true;
}

bool lobby_manager::is_window() const
{
    return IsWindow(reinterpret_cast<HWND>(window_)) ? true : false;
}

void lobby_manager::register_sng()
{
    if (registering_)
        return;

    if (!window_utils::click_any_button(input_manager_, window_, register_buttons_))
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

void lobby_manager::reset()
{
    registered_ = 0;
    registering_ = false;
    window_ = 0;
}

bool lobby_manager::ensure_visible()
{
    const auto hwnd = reinterpret_cast<HWND>(window_);

    if (IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd))
        return true;

    ShowWindowAsync(hwnd, SW_RESTORE);

    return false;
}
