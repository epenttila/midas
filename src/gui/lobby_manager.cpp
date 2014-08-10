#include "lobby_manager.h"

#pragma warning(push, 1)
#include <boost/log/trivial.hpp>
#include <QTime>
#include <boost/range.hpp>
#pragma warning(pop)

#include "input_manager.h"
#include "fake_window.h"
#include "site_settings.h"

namespace
{
    std::string get_table_string(const std::unordered_set<lobby_manager::tid_t>& tables)
    {
        std::string s;

        for (const auto& i : tables)
        {
            if (!s.empty())
                s += ",";

            s += std::to_string(i);
        }

        return "[" + s + "]";
    }

    bool close_popups(input_manager& input, fake_window& window, const site_settings::popup_range& popups)
    {
        if (!window.is_valid())
            return false;

        const auto title = window.get_window_text();

        for (const auto& i : popups)
        {
            if (std::regex_match(title, i.second->regex))
            {
                if (window.click_button(input, i.second->button))
                    return true;
            }
        }

        throw std::runtime_error(QString("Unable to close popup: %1").arg(title.c_str()).toStdString().c_str());
    }
}

lobby_manager::lobby_manager(const site_settings& settings, input_manager& input_manager, const window_manager& wm)
    : input_manager_(input_manager)
    , settings_(&settings)
{
    lobby_window_.reset(new fake_window(*settings_->get_window("lobby"), settings, wm));
    popup_window_.reset(new fake_window(*settings_->get_window("popup"), settings, wm));

    for (const auto& w : settings_->get_windows("table"))
        table_windows_.push_back(std::unique_ptr<fake_window>(new fake_window(*w.second, settings, wm)));
}

void lobby_manager::detect_closed_tables(const std::unordered_set<tid_t>& new_active_tables)
{
    if (new_active_tables == active_tables_)
        return; // no change

    const auto old_tables = active_tables_;

    active_tables_ = new_active_tables;
    table_update_time_.start();

    BOOST_LOG_TRIVIAL(info) << QString("Updating active tables (%1 -> %2)")
        .arg(get_table_string(old_tables).c_str()).arg(get_table_string(active_tables_).c_str()).toStdString();
}

const lobby_manager::table_vector_t& lobby_manager::get_tables() const
{
    return table_windows_;
}

void lobby_manager::update_windows()
{
    if (!lobby_window_ || !popup_window_)
        return;

    lobby_window_->update();
    popup_window_->update();

    for (auto& i : table_windows_)
        i->update();
}

lobby_manager::tid_t lobby_manager::get_tournament_id(const fake_window& window) const
{
    const auto title = window.get_window_text();
    std::smatch match;

    if (std::regex_match(title, match, *settings_->get_regex("tournament")))
        return std::stoll(match[1].str());

    throw std::runtime_error("Unknown tournament id");
}

bool lobby_manager::check_idle(const bool schedule_active)
{
    if (schedule_active)
    {
        if (table_update_time_.isNull())
            table_update_time_.start();

        const auto max_idle_time = settings_->get_number("max-idle-time");

        if (max_idle_time && table_update_time_.elapsed() > *max_idle_time * 1000.0)
        {
            BOOST_LOG_TRIVIAL(warning) << "We are idle";
            table_update_time_ = QTime();
            return true;
        }
    }
    else 
        table_update_time_ = QTime();

    return false;
}
