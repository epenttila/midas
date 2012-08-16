#include "lobby_888.h"

#pragma warning(push, 3)
#define NOMINMAX
#include <Windows.h>
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
    const auto window = GetForegroundWindow();

    if (!IsWindow(window))
        return;

    const auto title = window_manager::get_window_text(window);

    std::regex r_trny_reg("Tournament Registration: #\\d+");
    std::regex r_trny_reg_failed("Registration to tournament \\d+ failed\\.");
    std::regex r_trny_id("Tournament ID : \\d+");

    if (title == "User Message")
    {
        input_manager_.send_keypress(VK_RETURN);
    }
    else if (title == "Member Message")
    {
        input_manager_.send_keypress(VK_RETURN);
    }
    else if (std::regex_match(title, r_trny_reg))
    {
        input_manager_.send_keypress(VK_RETURN);
    }
    else if (std::regex_match(title, r_trny_id))
    {
        input_manager_.send_keypress(VK_RETURN);
        ++registered_;
    }
    else if (std::regex_match(title, r_trny_reg_failed))
    {
        input_manager_.send_keypress(VK_RETURN);
    }
    else if (title == "Rematch")
    {
        input_manager_.send_keypress(VK_ESCAPE);
        --registered_;
        assert(registered_ >= 0);
    }
    // TODO close ie windows? CloseWindow
}

bool lobby_888::is_window() const
{
    return IsWindow(window_) ? true : false;
}

void lobby_888::register_sng()
{
    const auto window = GetForegroundWindow();

    if (!IsWindow(window))
        return;

    if (window != window_)
        return;

    const auto image = window_manager::screenshot(window).toImage();

    input_manager_.move_mouse(window_, 13, 244, 731, 13);
    input_manager_.sleep();
    input_manager_.left_click();
    input_manager_.sleep();
    input_manager_.move_mouse(window_, 767, 565, 108, 28);
    input_manager_.sleep();

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
