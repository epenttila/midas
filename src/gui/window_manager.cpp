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
        return !wm->try_window(reinterpret_cast<WId>(hwnd));
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
    stop_ = segment_.find_or_construct<bool>("stop")();
}

window_manager::~window_manager()
{
    clear_window();
}

bool window_manager::is_window() const
{
    return IsWindow(reinterpret_cast<HWND>(window_)) ? true : false;
}

bool window_manager::find_window()
{
    if (is_window())
        return false; // consider the function result a failure if we already have a window

    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex_);

    set_->erase(window_);
    window_ = 0;

    EnumWindows(callback, reinterpret_cast<LPARAM>(this));

    return is_window();
}

WId window_manager::get_window() const
{
    return window_;
}

bool window_manager::is_window_good(const WId window) const
{
    if (!std::regex_match(get_window_text(window), title_regex_))
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

bool window_manager::try_window(WId window)
{
    if (!is_window_good(window))
        return false;

    // called from find_window() where the mutex is locked
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
    GetClassName(reinterpret_cast<HWND>(window), &arr[0], int(arr.size()));
    return std::string(arr.data());
}

std::string window_manager::get_window_text(WId window)
{
    std::array<char, 256> arr;
    GetWindowText(reinterpret_cast<HWND>(window), &arr[0], int(arr.size()));
    return std::string(arr.data());
}

Q_GUI_EXPORT QPixmap qt_pixmapFromWinHBITMAP(HBITMAP, int hbitmapFormat = 0);

QPixmap window_manager::screenshot(WId window)
{
    const auto hwnd = reinterpret_cast<HWND>(window);

    RECT r;
    GetClientRect(hwnd, &r);

    const int w = r.right - r.left;
    const int h = r.bottom - r.top;

    const HDC display_dc = GetDC(0);
    const HDC bitmap_dc = CreateCompatibleDC(display_dc);
    const HBITMAP bitmap = CreateCompatibleBitmap(display_dc, w, h);
    const HGDIOBJ null_bitmap = SelectObject(bitmap_dc, bitmap);

    const HDC window_dc = GetDC(hwnd);
    BitBlt(bitmap_dc, 0, 0, w, h, window_dc, 0, 0, SRCCOPY);

    ReleaseDC(hwnd, window_dc);
    SelectObject(bitmap_dc, null_bitmap);
    DeleteDC(bitmap_dc);

    const QPixmap pixmap = qt_pixmapFromWinHBITMAP(bitmap);

    DeleteObject(bitmap);
    ReleaseDC(0, display_dc);

    return pixmap;
}

void window_manager::stop()
{
    *stop_ = true;
}

bool window_manager::is_stop() const
{
    return *stop_;
}

std::unordered_set<WId> window_manager::get_tables() const
{
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex_);

    return std::unordered_set<WId>(set_->begin(), set_->end());
}
