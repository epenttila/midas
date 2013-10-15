#pragma once

#pragma warning(push, 1)
#include <regex>
#include <unordered_set>
#include <QWidget>
#pragma warning(pop)

class window_manager
{
public:
    window_manager();
    ~window_manager();
    void update();
    bool is_window_good(WId window) const;
    void set_title_filter(const std::string& filter);
    void try_window(WId window);
    void clear_window();
    std::unordered_set<WId> get_tables() const;

private:
    std::string title_filter_;
    std::regex title_regex_;
    std::unordered_set<WId> windows_;
};
