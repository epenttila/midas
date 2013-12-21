#include "lobby_manager.h"

#pragma warning(push, 3)
#include <boost/log/trivial.hpp>
#include <QXmlStreamReader>
#include <QFile>
#include <QTime>
#include <CommCtrl.h>
#pragma warning(pop)

#include "input_manager.h"
#include "table_manager.h"
#include "fake_window.h"

lobby_manager::lobby_manager(const std::string& filename, input_manager& input_manager)
    : input_manager_(input_manager)
    , registered_(0)
    , registration_wait_(5.0)
    , popup_wait_(5.0)
    , filename_(filename)
    , table_count_(0)
{
    BOOST_LOG_TRIVIAL(info) << "Loading lobby settings: " << filename;

    QFile file(filename.c_str());

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        throw std::runtime_error("unable to open xml file");

    QXmlStreamReader reader(&file);

    while (reader.readNextStartElement())
    {
        if (reader.name() == "site")
        {
            // do nothing
        }
        else if (reader.name() == "reg-success-popup")
        {
            reg_success_popups_.push_back(window_utils::read_xml_popup(reader));
        }
        else if (reader.name() == "reg-fail-popup")
        {
            reg_fail_popups_.push_back(window_utils::read_xml_popup(reader));
        }
        else if (reader.name() == "register-button")
        {
            register_buttons_.push_back(window_utils::read_xml_button(reader));
        }
        else if (reader.name() == "popup-wait")
        {
            popup_wait_ = reader.attributes().value("time").toDouble();
            reader.skipCurrentElement();
        }
        else if (reader.name() == "registration-wait")
        {
            registration_wait_ = reader.attributes().value("time").toDouble();
            reader.skipCurrentElement();
        }
        else if (reader.name() == "lobby-window")
        {
            lobby_window_.reset(new fake_window(0, window_utils::read_xml_rect(reader)));
        }
        else if (reader.name() == "popup-window")
        {
            popup_window_.reset(new fake_window(0, window_utils::read_xml_rect(reader)));
        }
        else if (reader.name() == "table-window")
        {
            table_windows_.push_back(std::unique_ptr<fake_window>(new fake_window(
                static_cast<int>(table_windows_.size()), window_utils::read_xml_rect(reader))));
        }
        else
        {
            reader.skipCurrentElement();
        }
    }
}

void lobby_manager::register_sng()
{
    if (!lobby_window_->click_any_button(input_manager_, register_buttons_))
        return;

    BOOST_LOG_TRIVIAL(info) << "Registering... (" << registered_ << " active)";

    if (reg_success_popups_.empty())
    {
        ++registered_;
        BOOST_LOG_TRIVIAL(info) << "Registration assumed successful (" << registered_ << " active)";
        return;
    }

    BOOST_LOG_TRIVIAL(info) << "Waiting " << registration_wait_ << " seconds for registration to complete";

    QTime t;
    t.start();

    while (t.elapsed() <= registration_wait_ * 1000)
    {
        if (close_popups(input_manager_, *popup_window_, reg_success_popups_, popup_wait_))
        {
            ++registered_;

            BOOST_LOG_TRIVIAL(info) << "Registration success (" << registered_ << " active)";
            return;
        }

        if (close_popups(input_manager_, *popup_window_, reg_fail_popups_, popup_wait_))
        {
            BOOST_LOG_TRIVIAL(info) << "Registration failed (" << registered_ << " active)";
            return;
        }
    }

    throw std::runtime_error("Unable to verify registration result");
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

bool lobby_manager::detect_closed_tables()
{
    const auto new_table_count = get_active_tables();
    bool ret = false;

    if (registered_ >= new_table_count)
    {
        if (new_table_count < table_count_)
        {
            registered_ -= table_count_ - new_table_count;
            BOOST_LOG_TRIVIAL(info) << "Tournament finished (" << registered_ << " active)";
        }

        if (registered_ < 0)
        {
            BOOST_LOG_TRIVIAL(warning) << "Negative registrations";
            registered_ = 0;
        }

        ret = true;
    }
    else
    {
        // wait until we get rid of the extra tables
        BOOST_LOG_TRIVIAL(warning) << "There are more tables than registrations";
        ret = false;
    }

    table_count_ = new_table_count;

    return ret;
}

int lobby_manager::get_active_tables() const
{
    int count = 0;

    for (const auto& i : table_windows_)
    {
        if (i->is_valid())
            ++count;
    }

    return count;
}

double lobby_manager::get_registration_wait() const
{
    return registration_wait_;
}

std::string lobby_manager::get_filename() const
{
    return filename_;
}

const lobby_manager::table_vector_t& lobby_manager::get_tables() const
{
    return table_windows_;
}

bool lobby_manager::close_popups(input_manager& input, fake_window& window,
    const std::vector<window_utils::popup_data>& popups, const double max_wait)
{
    if (!window.update())
        return false;

    const auto title = window.get_window_text();
    bool found = false;

    for (auto i = popups.begin(); i != popups.end(); ++i)
    {
        if (std::regex_match(title, i->regex))
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
        for (auto i = popups.begin(); i != popups.end(); ++i)
        {
            if (!std::regex_match(title, i->regex))
                continue;

            if (i->button.rect.isValid())
            {
                window.click_button(input, i->button);
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

void lobby_manager::update_windows(WId wid)
{
    if (!lobby_window_ || !popup_window_)
        return;

    lobby_window_->update(wid);
    popup_window_->update(wid);

    for (auto& i : table_windows_)
        i->update(wid);
}
