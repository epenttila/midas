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

    bool close_popups(input_manager& input, fake_window& window, const site_settings::popup_range& popups,
        const double max_wait)
    {
        if (!window.update())
            return false;

        const auto title = window.get_window_text();
        bool found = false;

        for (const auto& i : popups)
        {
            if (std::regex_match(title, i.second->regex))
            {
                found = true;
                break;
            }
        }

        if (!found)
            return false;

        // not all windows will be closed when clicking OK, so can't use IsWindow here
        QTime t;
        t.start();

        const int wait = static_cast<int>(max_wait * 1000);

        while (window.update())
        {
            for (const auto& i : popups)
            {
                if (!std::regex_match(title, i.second->regex))
                    continue;

                if (i.second->button.rect.isValid())
                {
                    window.click_button(input, i.second->button);
                    input.sleep();
                }
                else
                {
                    throw std::runtime_error("Do not know how to close popup");
                }
            }

            if (t.elapsed() > wait)
            {
                BOOST_LOG_TRIVIAL(warning) << "Failed to close window after " << t.elapsed() << " ms (" << title << ")";
                return false;
            }
        }

        return true;
    }
}

lobby_manager::lobby_manager(const site_settings& settings, input_manager& input_manager)
    : input_manager_(input_manager)
    , registered_(0)
    , settings_(&settings)
{
    lobby_window_.reset(new fake_window(*settings_->get_window("lobby"), settings));
    popup_window_.reset(new fake_window(*settings_->get_window("popup"), settings));

    for (const auto& w : settings_->get_windows("table"))
        table_windows_.push_back(std::unique_ptr<fake_window>(new fake_window(*w.second, settings)));
}

void lobby_manager::register_sng()
{
    lobby_window_->click_any_button(input_manager_, settings_->get_buttons("game-list"));

    if (!lobby_window_->click_any_button(input_manager_, settings_->get_buttons("register")))
        return;

    BOOST_LOG_TRIVIAL(info) << "Registering... (" << registered_ << " active)";

    const auto& registration_wait = *settings_->get_number("reg-wait");

    BOOST_LOG_TRIVIAL(info) << "Waiting " << registration_wait << " seconds for registration to complete";

    const auto& popup_wait = *settings_->get_number("popup-wait");

    QTime t;
    t.start();

    while (t.elapsed() <= registration_wait * 1000)
    {
        if (close_popups(input_manager_, *popup_window_, settings_->get_popups("reg-ok"), popup_wait))
        {
            ++registered_;

            BOOST_LOG_TRIVIAL(info) << "Registration success (" << registered_ << " active)";
            return;
        }
    }

    BOOST_LOG_TRIVIAL(warning) << "Unable to verify registration result";
}

int lobby_manager::get_registered_sngs() const
{
    return registered_;
}

void lobby_manager::reset()
{
    registered_ = 0;

    BOOST_LOG_TRIVIAL(info) << "Resetting registrations (" << registered_ << " active)";
}

void lobby_manager::detect_closed_tables()
{
    const auto new_active_tables = get_active_tables();
    const auto old_regs = registered_;

    if (new_active_tables == active_tables_)
        return; // no change

    if (new_active_tables.size() > registered_)
    {
        // assume we missed a registration confirmation
        registered_ = static_cast<int>(new_active_tables.size());
        active_tables_ = new_active_tables;

        BOOST_LOG_TRIVIAL(warning) << QString(
            "There are more tables than registrations (regs: %1 -> %2; tables: %3 -> %4)")
            .arg(old_regs).arg(registered_).arg(get_table_string(active_tables_).c_str())
            .arg(get_table_string(new_active_tables).c_str()).toStdString();

        return;
    }

    // find closed tables
    for (const auto& i : active_tables_)
    {
        if (new_active_tables.find(i) == new_active_tables.end())
        {
            --registered_;

            BOOST_LOG_TRIVIAL(info) << QString("Tournament finished (regs: %1 -> %2; tables: %3 -> %4)")
                .arg(old_regs).arg(registered_).arg(get_table_string(active_tables_).c_str())
                .arg(get_table_string(new_active_tables).c_str()).toStdString();
        }
    }

    if (registered_ < 0)
    {
        registered_ = 0;

        BOOST_LOG_TRIVIAL(warning) << QString("Negative registrations (regs: %1 -> %2; tables: %3 -> %4)")
            .arg(old_regs).arg(registered_).arg(get_table_string(active_tables_).c_str())
            .arg(get_table_string(new_active_tables).c_str()).toStdString();
    }

    active_tables_ = new_active_tables;
}

std::unordered_set<lobby_manager::tid_t> lobby_manager::get_active_tables() const
{
    std::unordered_set<tid_t> active;

    for (const auto& i : table_windows_)
    {
        if (i->is_valid())
            active.insert(get_tournament_id(*i));
    }

    return active;
}

const lobby_manager::table_vector_t& lobby_manager::get_tables() const
{
    return table_windows_;
}

void lobby_manager::update_windows(WId wid)
{
    if (!lobby_window_ || !popup_window_)
        return;

    lobby_window_->update(wid);
    popup_window_->update(wid);

    for (auto& i : table_windows_)
        i->update(wid);
}

lobby_manager::tid_t lobby_manager::get_tournament_id(const fake_window& window) const
{
    const auto title = window.get_window_text();
    std::smatch match;

    if (std::regex_match(title, match, *settings_->get_regex("tournament")))
        return std::stoll(match[1].str());

    throw std::runtime_error("Unknown tournament id");
}
