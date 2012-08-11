#pragma once

#include <string>
#include <random>

class input_manager
{
public:
    input_manager();
    void send_keypress(short vk);
    void send_string(const std::string& s);
    void set_delay_mean(double mean);
    void set_delay_stddev(double stddev);
    double get_delay_mean() const;
    double get_delay_stddev() const;
    void sleep();

private:
    std::mt19937 engine_;
    std::normal_distribution<> dist_;
};
