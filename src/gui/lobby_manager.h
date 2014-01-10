#pragma once

#pragma warning(push, 3)
#include <regex>
#include <unordered_set>
#include <memory>
#include <boost/noncopyable.hpp>
#include <QWidget>
#include <Windows.h>
#pragma warning(pop)

#include "window_utils.h"

class input_manager;
class fake_window;

class lobby_manager : private boost::noncopyable
{
public:
    typedef std::vector<std::unique_ptr<fake_window>> table_vector_t;
    typedef std::int64_t tid_t;

    lobby_manager(const std::string& filename, input_manager& input_manager);
    void register_sng();
    int get_registered_sngs() const;
    void reset();
    void detect_closed_tables();
    std::unordered_set<tid_t> get_active_tables() const;
    double get_registration_wait() const;
    std::string get_filename() const;
    const table_vector_t& get_tables() const;
    void update_windows(WId wid);
    tid_t get_tournament_id(const fake_window& window) const;

private:
    bool close_popups(input_manager& input, fake_window& window, const std::vector<window_utils::popup_data>& popups,
        const double max_wait);

    std::unique_ptr<fake_window> lobby_window_;
    input_manager& input_manager_;
    int registered_;
    double registration_wait_;
    double popup_wait_;
    std::vector<window_utils::popup_data> reg_success_popups_;
    std::vector<window_utils::button_data> register_buttons_;
    std::vector<window_utils::button_data> game_lists_;
    table_vector_t table_windows_;
    std::string filename_;
    std::unique_ptr<fake_window> popup_window_;
    std::unordered_set<tid_t> active_tables_;
    std::regex tournament_id_regex_;
};
