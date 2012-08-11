#include "input_manager.h"

#pragma warning(push, 1)
#include <Windows.h>
#pragma warning(pop)

input_manager::input_manager()
    : engine_(std::random_device()())
    , dist_(100, 10)
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

    Sleep(int(dist_(engine_)));

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
            Sleep(int(dist_(engine_)));

        send_keypress(*it);
    }
}
