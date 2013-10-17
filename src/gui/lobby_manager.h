#pragma once

#pragma warning(push, 3)
#include <regex>
#include <unordered_set>
#include <boost/noncopyable.hpp>
#include <QWidget>
#include <Windows.h>
#pragma warning(pop)

#include "window_utils.h"

class input_manager;
class window_manager;

class lobby_manager : private boost::noncopyable
{
public:
    lobby_manager(const std::string& filename, input_manager& input_manager, const window_manager& window_manager);
    bool close_popups();
    bool is_window() const;
    bool register_sng();
    int get_registered_sngs() const;
    void set_window(WId window);
    void reset();
    bool ensure_visible();
    void cancel_registration();
    bool detect_closed_tables();
    int get_table_count() const;
    double get_registration_wait() const;
    QString get_lobby_pattern() const;
    std::string get_filename() const;

private:
    static BOOL CALLBACK callback(HWND window, LPARAM lParam);

    WId window_;
    input_manager& input_manager_;
    int registered_;
    bool registering_;
    const window_manager& window_manager_;
    double registration_wait_;
    double popup_wait_;

    std::vector<window_utils::popup_data> popups_;
    std::vector<window_utils::popup_data> reg_fail_popups_;
    std::vector<window_utils::popup_data> reg_success_popups_;
    std::vector<window_utils::popup_data> finished_popups_;

    std::vector<window_utils::button_data> register_buttons_;

    std::unordered_set<WId> tables_;
    QString lobby_pattern_;

    std::string filename_;
};
