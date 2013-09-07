#pragma once

#pragma warning(push, 3)
#include <regex>
#include <boost/noncopyable.hpp>
#include <QWidget>
#define NOMINMAX
#include <Windows.h>
#pragma warning(pop)

#include "window_utils.h"

class input_manager;

class lobby_manager : private boost::noncopyable
{
public:
    lobby_manager(const std::string& filename, input_manager& input_manager);
    void close_popups();
    bool is_window() const;
    void register_sng();
    int get_registered_sngs() const;
    void set_window(WId window);

private:
    struct confirm_data
    {
        std::regex regex;
        QRect rect;
    };

    static BOOL CALLBACK callback(HWND window, LPARAM lParam);

    WId window_;
    input_manager& input_manager_;
    int registered_;
    bool registering_;

    std::vector<std::regex> popup_regexes_;
    std::vector<std::regex> registered_regexes_;
    std::vector<std::regex> unregistered_regexes_;
    std::vector<confirm_data> confirm_regexes_;

    std::vector<window_utils::button_data> game_list_buttons_;
    std::vector<window_utils::button_data> unregister_buttons_;
    std::vector<window_utils::button_data> register_buttons_;
};
