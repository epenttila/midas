#include "input_manager.h"

#pragma warning(push, 1)
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
