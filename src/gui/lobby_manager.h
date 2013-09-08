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
    void reset();
    bool ensure_visible();

private:
    static BOOL CALLBACK callback(HWND window, LPARAM lParam);

    WId window_;
    input_manager& input_manager_;
    int registered_;
    bool registering_;

    std::vector<window_utils::popup_data> popups_;
    std::vector<window_utils::popup_data> reg_fail_popups_;
    std::vector<window_utils::popup_data> reg_success_popups_;
    std::vector<window_utils::popup_data> finished_popups_;

    std::vector<window_utils::button_data> register_buttons_;
};
