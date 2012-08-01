#pragma once

#pragma warning(push, 1)
#include <regex>
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <QWidget>
#pragma warning(pop)

class window_manager
{
public:
    window_manager();
    ~window_manager();
    bool is_window() const;
    bool find_window();
    WId get_window() const;
    bool is_window_good(WId window) const;
    void set_class_filter(const std::string& filter);
    void set_title_filter(const std::string& filter);
    bool try_window(WId window);
    std::string get_class_name() const;
    std::string get_title_name() const;

private:
    typedef WId key_type;
    typedef boost::interprocess::allocator<key_type, boost::interprocess::managed_windows_shared_memory::segment_manager> shm_allocator;
    typedef boost::interprocess::set<key_type, std::less<key_type>, shm_allocator> shm_set;

    void clear_window();

    WId window_;
    std::string class_filter_;
    std::string title_filter_;
    std::regex class_regex_;
    std::regex title_regex_;
    boost::interprocess::managed_windows_shared_memory segment_;
    shm_set* set_;
    boost::interprocess::interprocess_mutex* mutex_;
};
