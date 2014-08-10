#pragma once

#pragma warning(push, 1)
#include <unordered_set>
#include <memory>
#include <boost/noncopyable.hpp>
#include <QWidget>
#include <QTime>
#pragma warning(pop)

#include "site_settings.h"

class input_manager;
class fake_window;
class window_manager;

class lobby_manager : private boost::noncopyable
{
public:
    typedef std::vector<std::unique_ptr<fake_window>> table_vector_t;
    typedef std::int64_t tid_t;

    lobby_manager(const site_settings& settings, input_manager& input_manager, const window_manager& wm);
    void detect_closed_tables(const std::unordered_set<tid_t>& new_active_tables);
    const table_vector_t& get_tables() const;
    void update_windows();
    tid_t get_tournament_id(const fake_window& window) const;
    bool check_idle(bool schedule_active);

private:
    std::unique_ptr<fake_window> lobby_window_;
    input_manager& input_manager_;
    table_vector_t table_windows_;
    std::unordered_set<tid_t> active_tables_;
    const site_settings* settings_;
    QTime table_update_time_;
};
