#include "input_manager.h"

#pragma warning(push, 1)
#include <boost/math/special_functions.hpp>
#include <boost/algorithm/clamp.hpp>
#include <QTime>
#include <Windows.h>
#pragma warning(pop)

#include "util/random.h"

input_manager::input_manager()
    : engine_(std::random_device()())
{
}

void input_manager::send_keypress(short vk)
{
    const auto scan = static_cast<WORD>(MapVirtualKeyEx(vk, MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));

    KEYBDINPUT kb = {};
    INPUT input = {};

    kb.wVk = vk;
    kb.wScan = scan;
    input.type = INPUT_KEYBOARD;
    input.ki = kb;
    SendInput(1, &input, sizeof(input));

    sleep();

    kb.wVk = vk;
    kb.wScan = scan;
    kb.dwFlags = KEYEVENTF_KEYUP;
    input.type = INPUT_KEYBOARD;
    input.ki = kb;
    SendInput(1, &input, sizeof(input));
}

void input_manager::send_string(const std::string& s)
{
    for (auto it = s.begin(); it != s.end(); ++it)
    {
        if (it != s.begin())
            sleep();

        send_keypress(*it);
    }
}

void input_manager::set_delay(double min, double max)
{
    delay_[0] = min;
    delay_[1] = max;
}

void input_manager::sleep()
{
    Sleep(int(get_random_delay() * 1000.0));
}

void input_manager::set_cursor_position(int x, int y)
{
    const int width = GetSystemMetrics(SM_CXSCREEN); 
    const int height = GetSystemMetrics(SM_CYSCREEN); 
    const int xx = boost::math::iround(x * 65535.0 / (width - 1));
    const int yy = boost::math::iround(y * 65535.0 / (height - 1));

    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    input.mi.dx = xx;
    input.mi.dy = yy;
    SendInput(1, &input, sizeof(input));
}

void input_manager::wind_mouse_impl(double xs, double ys, double xe, double ye, double gravity, double wind, double min_wait,
    double max_wait, double max_step, double target_area)
{
    std::uniform_real_distribution<> distribution(0.0, 1.0);
    const double sqrt3 = std::sqrt(3.0);
    const double sqrt5 = std::sqrt(5.0);
    double distance;
    double velo_x = 0;
    double velo_y = 0;
    double wind_x = 0;
    double wind_y = 0;

    QTime time;
    time.start();

    while ((distance = boost::math::hypot(xs - xe, ys - ye)) >= 1)
    {
        if (time.elapsed() >= 10000)
            break;

        wind = std::min(wind, distance);

        if (distance >= target_area)
        {
            wind_x = wind_x / sqrt3 + (distribution(engine_) * (wind * 2.0 + 1.0) - wind) / sqrt5;
            wind_y = wind_y / sqrt3 + (distribution(engine_) * (wind * 2.0 + 1.0) - wind) / sqrt5;
        }
        else
        {
            wind_x /= sqrt3;
            wind_y /= sqrt3;

            if (max_step < 3)
                max_step = distribution(engine_) * 3 + 3.0;
            else
                max_step /= sqrt5;
        }

        velo_x += wind_x + gravity * (xe - xs) / distance;
        velo_y += wind_y + gravity * (ye - ys) / distance;
        const double velo_mag = boost::math::hypot(velo_x, velo_y);

        if (velo_mag > max_step)
        {
            double random_dist = max_step / 2.0 + distribution(engine_) * max_step / 2.0;
            velo_x = (velo_x / velo_mag) * random_dist;
            velo_y = (velo_y / velo_mag) * random_dist;
        }

        xs += velo_x;
        ys += velo_y;
        int mx = boost::math::iround(xs);
        int my = boost::math::iround(ys);
        POINT pt;
        GetCursorPos(&pt);

        if (pt.x != mx || pt.y != my)
            set_cursor_position(mx, my);

        const double step = boost::math::hypot(xs - pt.x, ys - pt.y);

        const auto wait = boost::math::iround(boost::algorithm::clamp(
            (max_wait - min_wait) * (step / max_step) + min_wait, min_wait, max_wait));

        Sleep(wait);
    }
}

void input_manager::move_mouse(int x, int y)
{
    POINT pt;
    GetCursorPos(&pt);

    if (pt.x == x && pt.y == y)
        return;

    const int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    x = boost::algorithm::clamp(x, left, left + width - 1);
    y = boost::algorithm::clamp(y, top, top + height - 1);

    const double speed = get_uniform_random(engine_, mouse_speed_[0], mouse_speed_[1]);

    wind_mouse_impl(pt.x, pt.y, x, y, 9, 3, 5.0 / speed, 10.0 / speed, 10.0 * speed, 8.0 * speed);
    SetCursorPos(x, y);

    GetCursorPos(&pt);

    if (pt.x == x && pt.y == y)
        return;

    throw std::runtime_error(QString("Mouse cursor (%1,%2) moved outside target (%3,%4)").arg(pt.x).arg(pt.y)
        .arg(x).arg(y).toStdString().c_str());
}

void input_manager::move_mouse(int x, int y, int width, int height, bool force)
{
    POINT pt;
    GetCursorPos(&pt);

    if (!force && pt.x >= x && pt.x < x + width && pt.y >= y && pt.y < y + height)
        return;

    int target_x = boost::math::iround(get_normal_random(engine_, x, x + width - 1));
    int target_y = boost::math::iround(get_normal_random(engine_, y, y + height - 1));

    target_x = boost::algorithm::clamp(target_x, x, x + width - 1);
    target_y = boost::algorithm::clamp(target_y, y, y + height - 1);

    move_mouse(target_x, target_y);

    GetCursorPos(&pt);

    if (pt.x >= x && pt.x < x + width && pt.y >= y && pt.y < y + height)
        return;

    throw std::runtime_error(QString("Mouse cursor (%1,%2) moved outside target (%3,%4,%5,%6)").arg(pt.x).arg(pt.y)
        .arg(x).arg(y).arg(x + width).arg(y + height).toStdString().c_str());
}

void input_manager::move_mouse(WId window, int x, int y, int width, int height)
{
    POINT point = {x, y};
    ClientToScreen(reinterpret_cast<HWND>(window), &point);
    move_mouse(point.x, point.y, width, height);
}

void input_manager::left_click()
{
    button_down();
    sleep();
    button_up();
}

void input_manager::button_down()
{
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
}

void input_manager::button_up()
{
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void input_manager::left_double_click()
{
    left_click();

    const auto val = static_cast<int>(get_normal_random(engine_, double_click_delay_[0], double_click_delay_[1])
        * 1000.0);

    Sleep(val);

    left_click();
}

double input_manager::get_random_delay()
{
    return get_normal_random(engine_, delay_[0], delay_[1]);
}

void input_manager::set_mouse_speed(double min, double max)
{
    mouse_speed_[0] = min;
    mouse_speed_[1] = max;
}

void input_manager::move_click(int x, int y, int width, int height, bool double_click)
{
    using namespace boost::math;

    move_mouse(x, y, width, height);
    sleep();

    if (double_click)
    {
        left_double_click();
    }
    else
    {
        button_down();
        sleep();

        const auto angle = std::uniform_real_distribution<>(0, 2 * constants::pi<double>())(engine_);
        const auto sin_angle = std::sin(angle);
        const auto cos_angle = std::cos(angle);
        const auto dist = std::uniform_real_distribution<>(0, 2)(engine_);

        POINT pt;
        GetCursorPos(&pt);

        const auto new_x = boost::algorithm::clamp(iround(pt.x + cos_angle * dist), x, x + width - 1);
        const auto new_y = boost::algorithm::clamp(iround(pt.y + sin_angle * dist), y, y + height - 1);

        set_cursor_position(new_x, new_y);

        button_up();
    }

    //dist = std::uniform_real_distribution<>(0, 500)(engine_);

    //move_mouse(iround(pt.x + cos_angle * dist), iround(pt.y + sin_angle * dist));
}

void input_manager::move_random(idle_move method)
{
    switch (method)
    {
    case IDLE_MOVE_DESKTOP:
        move_mouse(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), true);
        break;
    case IDLE_MOVE_OFFSET:
        {
            const auto angle = std::uniform_real_distribution<>(0, 2 * boost::math::constants::pi<double>())(engine_);
            const auto sin_angle = std::sin(angle);
            const auto cos_angle = std::cos(angle);
            const auto dist = std::uniform_real_distribution<>(0, 100)(engine_);

            POINT pt;
            GetCursorPos(&pt);

            const auto new_x = boost::math::iround(pt.x + cos_angle * dist);
            const auto new_y = boost::math::iround(pt.y + sin_angle * dist);

            move_mouse(new_x, new_y);
        }
        break;
    }
}

void input_manager::set_double_click_delay(double min, double max)
{
    double_click_delay_[0] = min;
    double_click_delay_[1] = max;
}
