#pragma once

#pragma warning(push, 1)
#include <string>
#include <random>
#include <array>
#include <QWidget>
#pragma warning(pop)

class input_manager
{
public:
    input_manager();
    void send_keypress(short vk);
    void send_string(const std::string& s);
    void set_delay(double min, double max);
    void sleep();
    void set_cursor_position(int x, int y);
    void move_mouse(int x, int y);
    void move_mouse(int x, int y, int width, int height);
    void move_mouse(WId window, int x, int y, int width, int height);
    void left_click();
    void left_double_click();
    void set_mouse_speed(double min, double max);

private:
    void wind_mouse_impl(double xs, double ys, double xe, double ye, double gravity, double wind, double min_wait,
        double max_wait, double max_step, double target_area);
    double get_random_delay();

    std::mt19937 engine_;
    std::array<double, 2> delay_;
    std::array<double, 2> mouse_speed_;
};
