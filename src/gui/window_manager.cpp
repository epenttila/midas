#include "window_manager.h"
#pragma warning(push, 1)
#include <regex>
#include <array>
#include <vector>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <Windows.h>
#pragma warning(pop)

namespace
{
    BOOL CALLBACK callback(HWND hwnd, LPARAM object)
    {
        window_manager* wm = reinterpret_cast<window_manager*>(object);
        return !wm->try_window(hwnd);
    }
}

window_manager::window_manager()
    : window_(0)
{
    using namespace boost::interprocess;

    segment_ = managed_windows_shared_memory(open_or_create, "midas", 65536);
    mutex_ = segment_.find_or_construct<interprocess_mutex>("window_list_mutex")();
    set_ = segment_.find_or_construct<shm_set>("window_list")(std::less<key_type>(),
        shm_allocator(segment_.get_segment_manager()));
    interact_mutex_ = segment_.find_or_construct<interprocess_mutex>("interact_mutex")();
}

window_manager::~window_manager()
{
    clear_window();
}

bool window_manager::is_window() const
{
    return IsWindow(window_) ? true : false;
}

bool window_manager::find_window()
{
    EnumWindows(callback, reinterpret_cast<LPARAM>(this));
    return is_window();
}

WId window_manager::get_window() const
{
    return window_;
}

bool window_manager::is_window_good(const WId window) const
{
    if (!std::regex_match(get_class_name(window), class_regex_))
        return false;

    if (!std::regex_match(get_window_text(window), title_regex_))
        return false;

    return true;
}

void window_manager::set_class_filter(const std::string& filter)
{
    if (filter == class_filter_)
        return;

    class_filter_ = filter;
    class_regex_ = std::regex(filter);
    clear_window();
}

void window_manager::set_title_filter(const std::string& filter)
{
    if (filter == title_filter_)
        return;

    title_filter_ = filter;
    title_regex_ = std::regex(filter);
    clear_window();
}

bool window_manager::try_window(WId window)
{
    if (!is_window_good(window))
        return false;

    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex_);

    if (set_->find(window) != set_->end())
        return false;

    set_->erase(window_);
    window_ = window;
    set_->insert(window_);

    return true;
}

std::string window_manager::get_class_name() const
{
    return get_class_name(window_);
}

std::string window_manager::get_title_name() const
{
    return get_window_text(window_);
}

void window_manager::clear_window()
{
    if (window_ == 0)
        return;

    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex_);
    set_->erase(window_);
    window_ = 0;
}

boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> window_manager::try_interact()
{
    return boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>(*interact_mutex_);
}

std::string window_manager::get_class_name(WId window)
{
    std::array<char, 256> arr;
    GetClassName(window, &arr[0], int(arr.size()));
    return std::string(arr.data());
}

std::string window_manager::get_window_text(WId window)
{
    std::array<char, 256> arr;
    GetWindowText(window, &arr[0], int(arr.size()));
    return std::string(arr.data());
}
