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
    const auto& registration_wait = *settings_->get_number("reg-wait");

    if (registration_time_.isNull())
    {
        lobby_window_->click_any_button(input_manager_, settings_->get_buttons("game-list"));

        if (!lobby_window_->click_any_button(input_manager_, settings_->get_buttons("register")))
            return;

        BOOST_LOG_TRIVIAL(info) << QString("Registering (waiting %1 seconds to complete)...").arg(registration_wait)
            .toStdString();

        registration_time_.start();
    }
    else if (registration_time_.elapsed() < registration_wait * 1000)
    {
        const auto& popup_wait = *settings_->get_number("popup-wait");

        if (close_popups(input_manager_, *popup_window_, settings_->get_popups("reg-ok"), popup_wait))
        {
            registration_time_ = QTime();
            ++registered_;

            BOOST_LOG_TRIVIAL(info) << QString("Registration success (regs: %1 -> %2)").arg(registered_ - 1)
                .arg(registered_).toStdString();
        }
    }
    else
    {
        registration_time_ = QTime();

        BOOST_LOG_TRIVIAL(warning) << "Unable to verify registration result";
    }
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

void lobby_manager::detect_closed_tables(const std::unordered_set<tid_t>& new_active_tables)
{
    if (registration_time_.isValid())
        return;

    if (new_active_tables == active_tables_)
        return; // no change

    const auto old_regs = registered_;
    const auto old_tables = active_tables_;

    active_tables_ = new_active_tables;

    BOOST_LOG_TRIVIAL(info) << QString("Updating active tables (%1 -> %2)")
        .arg(get_table_string(old_tables).c_str()).arg(get_table_string(active_tables_).c_str()).toStdString();

    if (active_tables_.size() > registered_)
    {
        // assume we missed a registration confirmation
        registered_ = static_cast<int>(active_tables_.size());
        registration_time_ = QTime();

        BOOST_LOG_TRIVIAL(warning) << QString("There are more tables than registrations (%1 -> %2)").arg(old_regs)
            .arg(registered_).toStdString();

        return;
    }

    // find closed tables
    for (const auto& i : old_tables)
    {
        if (active_tables_.find(i) == active_tables_.end())
        {
            --registered_;

            BOOST_LOG_TRIVIAL(info) << QString("Tournament %3 finished (%1 -> %2)").arg(old_regs).arg(registered_)
                .arg(i).toStdString();
        }
    }

    if (registered_ < 0)
    {
        registered_ = 0;

        BOOST_LOG_TRIVIAL(warning) << QString("Negative registrations (%1 -> %2)").arg(old_regs).arg(registered_)
            .toStdString();
    }
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

bool lobby_manager::is_registering() const
{
    return !registration_time_.isNull();
}
