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
    , registered_(0)
    , settings_(&settings)
    , state_(REGISTER)
{
    lobby_window_.reset(new fake_window(*settings_->get_window("lobby"), settings, wm));
    popup_window_.reset(new fake_window(*settings_->get_window("popup"), settings, wm));

    for (const auto& w : settings_->get_windows("table"))
        table_windows_.push_back(std::unique_ptr<fake_window>(new fake_window(*w.second, settings, wm)));
}

void lobby_manager::register_sng()
{
    const auto& registration_wait = *settings_->get_number("reg-wait");

    switch (state_)
    {
    case REGISTER:
        {
            lobby_window_->click_any_button(input_manager_, settings_->get_buttons("game-list"));

            if (!lobby_window_->click_any_button(input_manager_, settings_->get_buttons("register")))
                return;

            BOOST_LOG_TRIVIAL(info) << QString("Registering (waiting %1 seconds to complete)...").arg(registration_wait)
                .toStdString();

            registration_time_.start();

            state_ = CLOSE_POPUP;
        }
        break;
    case CLOSE_POPUP:
        {
            if (popup_window_->is_valid())
            {
                if (close_popups(input_manager_, *popup_window_, settings_->get_popups("reg-ok")))
                {
                    state_ = WAIT_POPUP;

                    BOOST_LOG_TRIVIAL(info) << "Closing registration popup";
                }
                else
                    throw std::runtime_error("Unable to close popup");
            }
            else if (registration_time_.elapsed() > registration_wait * 1000)
            {
                state_ = REGISTER;
                registration_time_ = QTime();

                BOOST_LOG_TRIVIAL(warning) << "Unable to verify registration result";
            }
        }
        break;
    case WAIT_POPUP:
        {
            if (popup_window_->is_valid())
            {
                state_ = CLOSE_POPUP;

                BOOST_LOG_TRIVIAL(info) << "Registration popup still open";
            }
            else
            {
                state_ = REGISTER;
                registration_time_ = QTime();
                ++registered_;

                BOOST_LOG_TRIVIAL(info) << QString("Registration success (regs: %1 -> %2)").arg(registered_ - 1)
                    .arg(registered_).toStdString();
            }
        }
        break;
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

    const auto old_tables = active_tables_;

    active_tables_ = new_active_tables;

    BOOST_LOG_TRIVIAL(info) << QString("Updating active tables (%1 -> %2)")
        .arg(get_table_string(old_tables).c_str()).arg(get_table_string(active_tables_).c_str()).toStdString();

    // find closed tables
    for (const auto& i : old_tables)
    {
        if (active_tables_.find(i) == active_tables_.end())
        {
            const auto old_regs = registered_;
            --registered_;

            BOOST_LOG_TRIVIAL(info) << QString("Tournament %3 finished (%1 -> %2)").arg(old_regs).arg(registered_)
                .arg(i).toStdString();
        }
    }

    if (registered_ < 0)
    {
        const auto old_regs = registered_;
        registered_ = 0;

        BOOST_LOG_TRIVIAL(warning) << QString("Negative registrations (%1 -> %2)").arg(old_regs).arg(registered_)
            .toStdString();
    }

    // adjust registrations after closing old tables above so we notice if a single table has been replaced,
    // e.g., table count remains the same (1 -> 1), but the tournament id of that table has changed
    if (active_tables_.size() > registered_)
    {
        // assume we missed a registration confirmation
        const auto old_regs = registered_;
        registered_ = static_cast<int>(active_tables_.size());
        registration_time_ = QTime();

        BOOST_LOG_TRIVIAL(warning) << QString("There are more tables than registrations (%1 -> %2)").arg(old_regs)
            .arg(registered_).toStdString();
    }
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

bool lobby_manager::is_registering() const
{
    return !registration_time_.isNull();
}
