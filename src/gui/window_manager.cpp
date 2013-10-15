#include "window_manager.h"
#pragma warning(push, 1)
#include <regex>
#include <array>
#include <vector>
#include <boost/log/trivial.hpp>
#include <Windows.h>
#pragma warning(pop)

#include "window_utils.h"

namespace
{
    BOOL CALLBACK callback(HWND hwnd, LPARAM object)
    {
        window_manager* wm = reinterpret_cast<window_manager*>(object);
        wm->try_window(reinterpret_cast<WId>(hwnd));
        return true;
    }
}

window_manager::window_manager()
{
}

window_manager::~window_manager()
{
    clear_window();
}

void window_manager::update()
{
    std::unordered_set<WId> remove;

    for (auto window : windows_)
    {
        if (!window_utils::is_window(window))
            remove.insert(window);
    }

    for (auto window : remove)
    {
        windows_.erase(window);
        BOOST_LOG_TRIVIAL(info) << QString("Window lost: %1").arg(window).toStdString();
    }

    EnumWindows(callback, reinterpret_cast<LPARAM>(this));
}

bool window_manager::is_window_good(const WId window) const
{
    const auto title = window_utils::get_window_text(window);

    if (title.empty())
        return false;

    if (!std::regex_match(title, title_regex_))
        return false;

    return true;
}

void window_manager::set_title_filter(const std::string& filter)
{
    if (filter == title_filter_)
        return;

    title_filter_ = filter;
    title_regex_ = std::regex(filter);
    clear_window();
}

void window_manager::try_window(WId window)
{
    if (!is_window_good(window))
        return;

    if (windows_.find(window) != windows_.end())
        return;

    windows_.insert(window);

    BOOST_LOG_TRIVIAL(info) << QString("Window found: %1 (%2)").arg(window)
        .arg(window_utils::get_window_text(window).c_str()).toStdString();
}

void window_manager::clear_window()
{
    windows_.clear();
}

std::unordered_set<WId> window_manager::get_tables() const
{
    return windows_;
}
