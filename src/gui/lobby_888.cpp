#include "lobby_888.h"

#pragma warning(push, 3)
#include <CommCtrl.h>
#pragma warning(pop)

#include "window_manager.h"
#include "input_manager.h"

lobby_888::lobby_888(WId window, input_manager& input_manager)
    : window_(window)
    , input_manager_(input_manager)
    , registered_(0)
{
}

void lobby_888::close_popups()
{
    EnumWindows(callback, reinterpret_cast<LPARAM>(this));
}

BOOL CALLBACK lobby_888::callback(HWND window, LPARAM lParam)
{
    if (!IsWindowVisible(window))
        return true;

    lobby_888* lobby = reinterpret_cast<lobby_888*>(lParam);

    RECT rect;
    GetClientRect(window, &rect);

    const auto title = window_manager::get_window_text(window);

    static std::regex r_trny_reg("Tournament Registration: #\\d+");
    static std::regex r_trny_reg_failed("Registration to tournament \\d+ failed\\.");
    static std::regex r_trny_id("Tournament ID : \\d+");
    static std::regex r_trny_id_unreg("Tournament ID: \\d+");

    if (title == "User Message")
    {
        SendMessage(window, WM_CLOSE, 0, 0);
    }
    else if (title == "Member Message")
    {
        SendMessage(window, WM_CLOSE, 0, 0);
    }
    else if (std::regex_match(title, r_trny_reg))
    {
        if (GetForegroundWindow() == window)
            lobby->input_manager_.send_keypress(VK_RETURN);
        else
            SendMessage(window, WM_CLOSE, 0, 0);
    }
    else if (std::regex_match(title, r_trny_id))
    {
        if (rect.right == 328 && rect.bottom == 206)
            ++lobby->registered_;

        SendMessage(window, WM_CLOSE, 0, 0);
    }
    else if (std::regex_match(title, r_trny_reg_failed))
    {
        SendMessage(window, WM_CLOSE, 0, 0);
    }
    else if (title == "Rematch")
    {
        SendMessage(window, WM_CLOSE, 0, 0);
        --lobby->registered_;
        assert(lobby->registered_ >= 0);
    }
    else if (std::regex_match(title, r_trny_id_unreg))
    {
        SendMessage(window, WM_CLOSE, 0, 0);
    }
    // TODO close ie windows? CloseWindow

    return true;
}

bool lobby_888::is_window() const
{
    return IsWindow(window_) ? true : false;
}

void lobby_888::register_sng()
{
    if (!IsWindow(window_))
        return;

    if (IsIconic(window_))
    {
        ShowWindowAsync(window_, SW_RESTORE);
        return;
    }

    auto image = window_manager::screenshot(window_).toImage();

    if ((image.pixel(767, 565) != qRgb(96, 168, 48) && image.pixel(767, 565) != qRgb(124, 181, 77))
        || image.pixel(786, 576) != qRgb(0, 0, 0))
    {
        return;
    }

    input_manager_.move_mouse(window_, 13, 244, 731, 13);
    input_manager_.sleep();
    input_manager_.left_click();
    input_manager_.sleep();
    input_manager_.move_mouse(window_, 767, 565, 108, 28);
    input_manager_.sleep();

    image = window_manager::screenshot(window_).toImage();

    if ((image.pixel(767, 565) != qRgb(96, 168, 48) && image.pixel(767, 565) != qRgb(124, 181, 77))
        || image.pixel(786, 576) != qRgb(0, 0, 0))
    {
        return;
    }

    input_manager_.left_click();
}

int lobby_888::get_registered_sngs() const
{
    return registered_;
}
