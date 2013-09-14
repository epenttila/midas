#include "input_manager.h"

#pragma warning(push, 1)
#include <boost/math/special_functions.hpp>
#include <boost/algorithm/clamp.hpp>
#include <Windows.h>
#pragma warning(pop)

input_manager::input_manager()
    : engine_(std::random_device()())
    , dist_(0.1, 0.01)
{
}

void input_manager::send_keypress(short vk)
{
    KEYBDINPUT kb = {};
    INPUT input = {};

    kb.wVk = vk;
    input.type = INPUT_KEYBOARD;
    input.ki = kb;
    SendInput(1, &input, sizeof(input));

    sleep();

    kb.wVk = vk;
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

void input_manager::set_delay_mean(double mean)
{
    dist_ = std::normal_distribution<>(mean, dist_.stddev());
}

void input_manager::set_delay_stddev(double stddev)
{
    dist_ = std::normal_distribution<>(dist_.mean(), stddev);
}

double input_manager::get_delay_mean() const
{
    return dist_.mean();
}

double input_manager::get_delay_stddev() const
{
    return dist_.stddev();
}

void input_manager::sleep()
{
    Sleep(int(dist_(engine_) * 1000.0));
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

    while ((distance = boost::math::hypot(xs - xe, ys - ye)) >= 1)
    {
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
        Sleep(boost::math::lround(((max_wait - min_wait) * (step / max_step) + min_wait)));
    }
}

void input_manager::move_mouse(int x, int y)
{
    const int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    x = boost::algorithm::clamp(x, left, left + width - 1);
    y = boost::algorithm::clamp(y, top, top + height - 1);

    POINT pt;
    GetCursorPos(&pt);

    std::uniform_real_distribution<> d(1.5, 3.0);
    const double speed = d(engine_);

    wind_mouse_impl(pt.x, pt.y, x, y, 9, 3, 5.0 / speed, 10.0 / speed, 10.0 * speed, 8.0 * speed);
    set_cursor_position(x, y);

#if !defined(NDEBUG)
    GetCursorPos(&pt);
    assert(pt.x == x && pt.y == y);
#endif
}

void input_manager::move_mouse(int x, int y, int width, int height)
{
    std::normal_distribution<> dist_x(x + width / 2.0, width / 10.0);
    std::normal_distribution<> dist_y(y + height / 2.0, height / 10.0);

    int target_x = boost::math::iround(dist_x(engine_));
    int target_y = boost::math::iround(dist_y(engine_));

    target_x = boost::algorithm::clamp(target_x, x, x + width - 1);
    target_y = boost::algorithm::clamp(target_y, y, y + height - 1);

    move_mouse(target_x, target_y);
}

void input_manager::move_mouse(WId window, int x, int y, int width, int height)
{
    POINT point = {x, y};
    ClientToScreen(reinterpret_cast<HWND>(window), &point);
    move_mouse(point.x, point.y, width, height);
}

void input_manager::left_click()
{
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));

    sleep();

    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}
