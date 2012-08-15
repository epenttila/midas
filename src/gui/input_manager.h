#pragma once

#pragma warning(push, 1)
#include <string>
#include <random>
#include <QWidget>
#pragma warning(pop)

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
    void set_cursor_position(int x, int y);
    void move_mouse(int x, int y);
    void move_mouse(int x, int y, int width, int height);
    void move_mouse(WId window, int x, int y, int width, int height);
    void left_click();

private:
    void wind_mouse_impl(double xs, double ys, double xe, double ye, double gravity, double wind, double min_wait,
        double max_wait, double max_step, double target_area);

    std::mt19937 engine_;
    std::normal_distribution<> dist_;
};
