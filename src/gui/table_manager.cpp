#include "table_manager.h"

#pragma warning(push, 1)
#include <boost/log/trivial.hpp>
#include <QImage>
#include <QXmlStreamReader>
#include <QFile>
#include <QDateTime>
#include <Windows.h>
#pragma warning(pop)

#include "util/card.h"
#include "util/game.h"
#include "input_manager.h"
#include "window_utils.h"
#include "fake_window.h"

table_manager::table_manager(const std::string& filename, input_manager& input_manager)
    : input_(input_manager)
{
    window_size_.fill(0);

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
        else if (reader.name() == "dealer-pixel")
        {
            const int id = reader.attributes().value("id").toString().toInt();
            dealer_pixels_[id] = window_utils::read_xml_pixel(reader);
        }
        else if (reader.name() == "fold-button")
        {
            fold_buttons_.push_back(window_utils::read_xml_button(reader));
        }
        else if (reader.name() == "call-button")
        {
            call_buttons_.push_back(window_utils::read_xml_button(reader));
        }
        else if (reader.name() == "raise-button")
        {
            raise_buttons_.push_back(window_utils::read_xml_button(reader));
        }
        else if (reader.name() == "title-bb-regex")
        {
            title_bb_regex_ = std::regex(reader.attributes().value("pattern").toUtf8());
            reader.skipCurrentElement();
        }
        else if (reader.name() == "sit-out-pixel")
        {
            const int id = reader.attributes().value("id").toString().toInt();
            sit_out_pixels_[id] = window_utils::read_xml_pixel(reader);
        }
        else if (reader.name() == "all-in-pixel")
        {
            const int id = reader.attributes().value("id").toString().toInt();
            all_in_pixels_[id] = window_utils::read_xml_pixel(reader);
        }
        else if (reader.name() == "bet-size-button")
        {
            const double fraction = reader.attributes().value("fraction").toString().toDouble();
            size_buttons_.insert(std::make_pair(fraction, window_utils::read_xml_button(reader)));
        }
        else if (reader.name() == "font")
        {
            const std::string id = reader.attributes().value("id").toUtf8();
            fonts_[id] = window_utils::read_xml_font(reader);
        }
        else if (reader.name() == "total-pot-label")
        {
            total_pot_label_ = window_utils::read_xml_label(reader);
        }
        else if (reader.name() == "bet-label")
        {
            const int id = reader.attributes().value("id").toString().toInt();
            bet_labels_[id] = window_utils::read_xml_label(reader);
        }
        else if (reader.name() == "stack-label")
        {
            const int id = reader.attributes().value("id").toString().toInt();
            stack_labels_[id] = window_utils::read_xml_label(reader);
        }
        else if (reader.name() == "stack-hilight-label")
        {
            const int id = reader.attributes().value("id").toString().toInt();
            flash_stack_labels_[id] = window_utils::read_xml_label(reader);
        }
        else if (reader.name() == "board-card")
        {
            const int id = reader.attributes().value("id").toString().toInt();
            board_card_rects_[id] = window_utils::read_xml_rect(reader);
        }
        else if (reader.name() == "suit")
        {
            const int id = reader.attributes().value("id").toString().toInt();
            suit_colors_[id] = QColor(reader.attributes().value("color").toString()).rgb();
            reader.skipCurrentElement();
        }
        else if (reader.name() == "card-color")
        {
            card_color_ = QColor(reader.attributes().value("color").toString()).rgb();
            reader.skipCurrentElement();
        }
        else if (reader.name() == "card-back-color")
        {
            card_color_ = QColor(reader.attributes().value("color").toString()).rgb();
            reader.skipCurrentElement();
        }
        else if (reader.name() == "hole-card")
        {
            const int id = reader.attributes().value("id").toString().toInt();
            hole_card_rects_[id] = window_utils::read_xml_rect(reader);
        }
        else if (reader.name() == "stack-hilight-pixel")
        {
            const int id = reader.attributes().value("id").toString().toInt();
            stack_hilight_pixels_[id] = window_utils::read_xml_pixel(reader);
        }
        else if (reader.name() == "bet-input-button")
        {
            bet_input_button_ = window_utils::read_xml_button(reader);
        }
        else if (reader.name() == "window-size")
        {
            window_size_[0] = reader.attributes().value("width").toString().toInt();
            window_size_[1] = reader.attributes().value("height").toString().toInt();
            reader.skipCurrentElement();
        }
        else
        {
            reader.skipCurrentElement();
        }
    }
}

void table_manager::update(const fake_window& window)
{
    // window contents have been updated previously in main_window
    window_.reset(new fake_window(window));

    image_.reset();
    mono_image_.reset();

    if (!window.is_valid())
        throw std::runtime_error(QString("Window %1 is invalid").arg(window_->get_id()).toStdString());

    if (!image_)
        image_.reset(new QImage);

    *image_ = window_->get_client_image();

    if (image_->isNull())
        throw std::runtime_error(QString("Capture for window %1 is invalid").arg(window_->get_id()).toStdString());

    if (!mono_image_)
        mono_image_.reset(new QImage);

    *mono_image_ = image_->convertToFormat(QImage::Format_Mono, Qt::ThresholdDither);
}

void table_manager::get_hole_cards(std::array<int, 2>& hole) const
{
    for (int i = 0; i < hole.size(); ++i)
    {
        hole[i] = parse_image_card(image_.get(), mono_image_.get(), hole_card_rects_[i], suit_colors_, card_color_,
            card_back_color_, fonts_.at("rank"));
    }
}

void table_manager::get_board_cards(std::array<int, 5>& board) const
{
    for (int i = 0; i < board_card_rects_.size(); ++i)
    {
        board[i] = parse_image_card(image_.get(), mono_image_.get(), board_card_rects_[i], suit_colors_, card_color_,
            card_back_color_, fonts_.at("rank"));
    }
}

int table_manager::get_dealer() const
{
    for (int i = 0; i < dealer_pixels_.size(); ++i)
    {
        if (is_pixel(image_.get(), dealer_pixels_[i]))
            return i;
    }

    return -1;
}

void table_manager::fold(double max_wait) const
{
    QTime t;
    t.start();

    while (!window_->click_any_button(input_, fold_buttons_))
    {
        if (t.elapsed() > max_wait * 1000.0)
        {
            throw std::runtime_error(QString("Unable to press fold button after %1 seconds")
                .arg(t.elapsed()).toStdString());
        }
    }
}

void table_manager::call(double max_wait) const
{
    QTime t;
    t.start();

    while (!window_->click_any_button(input_, call_buttons_))
    {
        if (t.elapsed() > max_wait * 1000.0)
        {
            throw std::runtime_error(QString("Unable to press call button after %1 seconds")
                .arg(t.elapsed()).toStdString());
        }
    }
}

void table_manager::raise(double amount, double fraction, double minbet, double max_wait) const
{
    if ((get_buttons() & RAISE_BUTTON) != RAISE_BUTTON)
    {
        // TODO move this out of here?
        BOOST_LOG_TRIVIAL(info) << "No raise button, calling instead";
        call(max_wait);
        return;
    }

    bool ok = (amount == minbet) ? true : false;

    // press bet size buttons
    if (!ok)
    {
        const auto range = size_buttons_.equal_range(fraction);

        for (auto i = range.first; i != range.second; ++i)
        {
            if (window_->click_button(input_, i->second))
            {
                input_.sleep();
                ok = true;
                break;
            }
        }
    }

    // type bet size manually
    if (!ok)
    {
        if (window_->click_button(input_, bet_input_button_, true))
        {
            input_.sleep();

            // TODO make sure we have focus
            // TODO support decimal point
            amount = std::max(std::min<double>(amount, std::numeric_limits<int>::max()), 0.0);
            input_.send_string(std::to_string(static_cast<int>(amount)));
            input_.sleep();
        }
        else if (fraction != ALLIN_BET_SIZE)
        {
            throw std::runtime_error("Unable to specify pot size");
        }
    }

    QTime t;
    t.start();

    while (!window_->click_any_button(input_, raise_buttons_))
    {
        if (t.elapsed() > max_wait * 1000.0)
        {
            throw std::runtime_error(QString("Unable to press raise button after %1 seconds")
                .arg(t.elapsed()).toStdString());
        }
    }
}

double table_manager::get_stack(int position) const
{
    const bool flashing = is_pixel(image_.get(), stack_hilight_pixels_[position]);
    const auto& label = flashing ? flash_stack_labels_[position] : stack_labels_[position];
    const auto s = parse_image_text(mono_image_.get(), label.rect, qRgb(255, 255, 255), fonts_.at(label.font));

    return s.empty() ? 0 : std::stof(s);
}

double table_manager::get_bet(int position) const
{
    return parse_image_bet(mono_image_.get(), bet_labels_[position], fonts_.at(bet_labels_[position].font));
}

double table_manager::get_big_blind() const
{
    const auto title = window_->get_window_text();
    std::smatch match;

    double big_blind = -1;

    if (std::regex_match(title, match, title_bb_regex_))
        big_blind = std::atof(match[1].str().c_str());

    return big_blind;
}

double table_manager::get_total_pot() const
{
    return parse_image_bet(mono_image_.get(), total_pot_label_, fonts_.at(total_pot_label_.font));
}

bool table_manager::is_all_in(int position) const
{
    return is_pixel(image_.get(), all_in_pixels_[position]);
}

int table_manager::get_buttons() const
{
    int buttons = 0;

    if (is_any_button(image_.get(), fold_buttons_))
        buttons |= FOLD_BUTTON;

    if (is_any_button(image_.get(), call_buttons_))
        buttons |= CALL_BUTTON;

    if (is_any_button(image_.get(), raise_buttons_))
        buttons |= RAISE_BUTTON;

    return buttons;
}

bool table_manager::is_sit_out(int position) const
{
    return is_pixel(image_.get(), sit_out_pixels_[position]);
}

bool table_manager::is_opponent_allin() const
{
    return is_all_in(1);
}

bool table_manager::is_opponent_sitout() const
{
    return is_sit_out(1);
}

void table_manager::save_snapshot() const
{
    if (!image_ || !mono_image_)
    {
        BOOST_LOG_TRIVIAL(warning) << "No image to save";
        return;
    }

    const auto filename = QDateTime::currentDateTimeUtc().toString("'snapshot-'yyyy-MM-ddTHHmmss.zzz'.png'");
    image_->save(filename);
    mono_image_->save("mono-" + filename);
}
