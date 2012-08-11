#pragma once

#include <string>
#include <random>

class input_manager
{
public:
    input_manager();
    void send_keypress(short vk);
    void send_string(const std::string& s);

private:
    std::mt19937 engine_;
    std::normal_distribution<> dist_;
};
