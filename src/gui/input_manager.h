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
    enum idle_move { IDLE_MOVE_DESKTOP, IDLE_MOVE_OFFSET, IDLE_NO_MOVE, MAX_IDLE_MOVES };

    input_manager();
    void send_keypress(short vk);
    void send_string(const std::string& s);
    void set_delay(double min, double max);
    void sleep();
    void set_cursor_position(int x, int y);
    void move_mouse(int x, int y);
    void move_mouse(int x, int y, int width, int height, bool force = false);
    void move_mouse(WId window, int x, int y, int width, int height);
    void left_click();
    void left_double_click();
    void set_mouse_speed(double min, double max);
    void button_down();
    void button_up();
    void move_click(int x, int y, int width, int height, bool double_click);
    void move_random(idle_move method);
    void set_double_click_delay(double min, double max);

private:
    void wind_mouse_impl(double xs, double ys, double xe, double ye, double gravity, double wind, double min_wait,
        double max_wait, double max_step, double target_area);
    double get_random_delay();

    std::mt19937 engine_;
    std::array<double, 2> delay_;
    std::array<double, 2> mouse_speed_;
    std::array<double, 2> double_click_delay_;
};
