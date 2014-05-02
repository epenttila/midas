#include "table_manager.h"

#pragma warning(push, 1)
#include <boost/log/trivial.hpp>
#include <unordered_set>
#include <QTime>
#include <sstream>
#pragma warning(pop)

#include "util/card.h"
#include "input_manager.h"
#include "window_utils.h"
#include "fake_window.h"

namespace
{
    int read_card(const fake_window& window, const QImage* image, const QImage* mono, const site_settings& settings, const std::string& id)
    {
        const auto& label = *settings.get_label(id);
        const auto& pixel = *settings.get_pixel(id);
        const auto& font = *settings.get_font(label.font);
        const std::array<site_settings::pixel_t, 4> suits = {
            *settings.get_pixel("club"),
            *settings.get_pixel("diamond"),
            *settings.get_pixel("heart"),
            *settings.get_pixel("spade"),
        };
        const auto& card_pixel = *settings.get_pixel("card");

        if (!image)
            return -1;

        const auto s = window_utils::read_string(mono, window.get_scaled_rect(label.unscaled_rect), label.color, font,
            label.tolerance, label.shift);

        if (s.empty())
            return -1;

        const auto avg = window_utils::get_average_color(*image, window.get_scaled_rect(pixel.unscaled_rect),
            card_pixel.color, card_pixel.tolerance);

        int suit = -1;

        for (int s = 0; s < suits.size(); ++s)
        {
            if (window_utils::get_color_distance(avg, suits[s].color) <= suits[s].tolerance)
            {
                if (suit != -1)
                {
                    throw std::runtime_error(QString("Unambiguous suit color (0x%1); was %2, could be %3")
                        .arg(avg, 0, 16).arg(suit).arg(s).toUtf8());
                }

                suit = s;
            }
        }

        if (suit == -1)
            throw std::runtime_error(QString("Unknown suit color (0x%1)").arg(avg, 0, 16).toUtf8());

        return get_card(string_to_rank(s), suit);
    }

    std::string read_label(const fake_window& window, const QImage* image, const site_settings& settings, const site_settings::label_t& label)
    {
        if (!image)
            return "";

        const auto s = window_utils::read_string(image, window.get_scaled_rect(label.unscaled_rect), label.color,
            *settings.get_font(label.font), label.tolerance, label.shift);

        std::smatch match;

        if (label.regex.mark_count() > 0 && std::regex_match(s, match, label.regex))
            return match[1].str();
  
        return s;
    }

    bool is_any_button(const fake_window& window, const site_settings::button_range& buttons)
    {
        for (const auto& i : buttons)
        {
            if (window.is_pixel(i.second->pixel))
                return true;
        }

        return false;
    }
}

table_manager::table_manager(const site_settings& settings, input_manager& input_manager)
    : input_(input_manager)
    , settings_(&settings)
    , window_(nullptr)
{
}

table_manager::snapshot_t table_manager::update(const fake_window& window)
{
    // window contents have been updated previously in main_window
    window_ = &window;

    image_.reset();
    mono_image_.reset();

    if (!window.is_valid())
        throw std::runtime_error("Invalid window");

    if (!image_)
        image_.reset(new QImage);

    *image_ = window_->get_client_image();

    if (image_->isNull())
        throw std::runtime_error("Invalid image");

    if (!mono_image_)
        mono_image_.reset(new QImage);

    *mono_image_ = image_->convertToFormat(QImage::Format_Mono, Qt::ThresholdDither);

    snapshot_t s;

    s.total_pot = get_total_pot();
    s.buttons = get_buttons();

    s.dealer[0] = is_dealer(0);
    s.dealer[1] = is_dealer(1);
    s.stack[0] = get_stack(0);
    s.stack[1] = get_stack(1);
    s.bet[0] = get_bet(0);
    s.bet[1] = get_bet(1);
    s.all_in[0] = is_all_in(0);
    s.all_in[1] = is_all_in(1);
    s.sit_out[0] = is_sit_out(0);
    s.sit_out[1] = is_sit_out(1);
    s.highlight[0] = is_highlight(0);
    s.highlight[1] = is_highlight(1);
    s.captcha = is_captcha();

    get_hole_cards(s.hole);
    get_board_cards(s.board);

    std::array<int, 7> cards = { s.hole[0], s.hole[1], s.board[0], s.board[1], s.board[2], s.board[3], s.board[4] };
    std::unordered_set<int> seen_cards;

    for (const auto& card : cards)
    {
        if (card == -1)
            continue;

        if (seen_cards.count(card) != 0)
        {
            const auto hole_string = get_card_string(s.hole[0]) + " " + get_card_string(s.hole[1]);
            const auto board_string =
                get_card_string(s.board[0]) + " "
                + get_card_string(s.board[1]) + " "
                + get_card_string(s.board[2]) + " "
                + get_card_string(s.board[3]) + " "
                + get_card_string(s.board[4]);

            throw std::runtime_error(QString("Duplicate card [%1] [%2]").arg(hole_string.c_str())
                .arg(board_string.c_str()).toUtf8());
        }

        seen_cards.insert(card);
    }

    return s;
}

void table_manager::get_hole_cards(std::array<int, 2>& hole) const
{
    static const std::array<const char*, 2> ids = { "hole-0", "hole-1" };

    for (int i = 0; i < hole.size(); ++i)
        hole[i] = read_card(*window_, image_.get(), mono_image_.get(), *settings_, ids[i]);
}

void table_manager::get_board_cards(std::array<int, 5>& board) const
{
    static const std::array<const char*, 5> ids = { "board-0", "board-1", "board-2", "board-3", "board-4" };

    for (int i = 0; i < board.size(); ++i)
        board[i] = read_card(*window_, image_.get(), mono_image_.get(), *settings_, ids[i]);
}

int table_manager::get_dealer_mask() const
{
    int dealer = 0;

    if (window_->is_pixel(*settings_->get_pixel("dealer-0")))
        dealer |= PLAYER;

    if (window_->is_pixel(*settings_->get_pixel("dealer-1")))
        dealer |= OPPONENT;

    return dealer;
}

bool table_manager::is_dealer(int position) const
{
    if (position == 0)
        return (get_dealer_mask() & PLAYER) == PLAYER;
    else if (position == 1)
        return (get_dealer_mask() & OPPONENT) == OPPONENT;

    return false;
}

void table_manager::fold(double max_wait) const
{
    QTime t;
    t.start();

    while (!window_->click_any_button(input_, settings_->get_buttons("fold")))
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

    while (!window_->click_any_button(input_, settings_->get_buttons("call")))
    {
        if (t.elapsed() > max_wait * 1000.0)
        {
            throw std::runtime_error(QString("Unable to press call button after %1 seconds")
                .arg(t.elapsed()).toStdString());
        }
    }
}

void table_manager::raise(const std::string& action, double amount, double minbet, double max_wait, raise_method method) const
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
        const auto& buttons = settings_->get_buttons(action);

        ok = window_->click_any_button(input_, buttons);

        if (ok)
            input_.sleep();

        if (!ok && !buttons.empty())
        {
            // TODO: throw?
            BOOST_LOG_TRIVIAL(warning) << "Unable to press bet size button";
        }
    }

    // type bet size manually
    if (!ok)
    {
        bool focused = false;

        const auto& focus_button = *settings_->get_button("focus");

        if (method == CLICK_TABLE && focus_button.unscaled_rect.isValid())
            focused = window_->click_button(input_, focus_button);

        if (!focused)
            focused = window_->click_any_button(input_, settings_->get_buttons("input"), true);

        if (focused)
        {
            input_.sleep();

            // TODO make sure we have focus
            // TODO support decimal point
            amount = std::max(std::min<double>(amount, std::numeric_limits<int>::max()), 0.0);
            input_.send_string(std::to_string(static_cast<int>(amount)));
            input_.sleep();
        }
        else if (action != "RAISE_A")
        {
            throw std::runtime_error("Unable to specify pot size");
        }
    }

    QTime t;
    t.start();

    while (!window_->click_any_button(input_, settings_->get_buttons("raise")))
    {
        if (t.elapsed() > max_wait * 1000.0)
        {
            throw std::runtime_error(QString("Unable to press raise button after %1 seconds")
                .arg(t.elapsed()).toStdString());
        }
    }
}

std::string table_manager::get_stack_text(int position) const
{
    static const std::array<const char*, 2> stack_ids = { "stack-0", "stack-1" };
    static const std::array<const char*, 2> active_stack_ids = { "active-stack-0", "active-stack-1" };

    const auto active_stack_label = settings_->get_label(active_stack_ids[position]);

    const auto label = (active_stack_label && is_highlight(position))
        ? active_stack_label
        : settings_->get_label(stack_ids[position]);

    return read_label(*window_, mono_image_.get(), *settings_, *label);
}

double table_manager::get_stack(int position) const
{
    const auto stack_allin = settings_->get_regex("stack-allin");
    const auto stack_sitout = settings_->get_regex("stack-sitout");
    const auto s = get_stack_text(position);

    if (s.empty()
        || (stack_allin && std::regex_match(s, *stack_allin))
        || (stack_sitout && std::regex_match(s, *stack_sitout)))
    {
        return 0;
    }

    return std::stof(s);
}

double table_manager::get_bet(int position) const
{
    static const std::array<const char*, 2> ids = { "bet-0", "bet-1" };

    const auto s = read_label(*window_, mono_image_.get(), *settings_, *settings_->get_label(ids[position]));

    return s.empty() ? 0 : std::stof(s);
}

double table_manager::get_total_pot() const
{
    const auto s = read_label(*window_, mono_image_.get(), *settings_, *settings_->get_label("total-pot"));
    const auto total = s.empty() ? 0 : std::stof(s);

    if (total > 0)
        return total;
    else
        return get_bet(0) + get_bet(1) + get_pot();
}

bool table_manager::is_all_in(int position) const
{
    static const std::array<const char*, 2> ids = { "allin-0", "allin-1" };

    if (const auto p = settings_->get_pixel(ids[position]))
        return window_->is_pixel(*p);
    else if (const auto p = settings_->get_regex("stack-allin"))
        return std::regex_match(get_stack_text(position), *p);
    else
        return false;
}

int table_manager::get_buttons() const
{
    int buttons = 0;

    if (is_any_button(*window_, settings_->get_buttons("fold")))
        buttons |= FOLD_BUTTON;

    if (is_any_button(*window_, settings_->get_buttons("call")))
        buttons |= CALL_BUTTON;

    if (is_any_button(*window_, settings_->get_buttons("raise")))
        buttons |= RAISE_BUTTON;

    if (is_any_button(*window_, settings_->get_buttons("input")))
        buttons |= INPUT_BUTTON;

    return buttons;
}

bool table_manager::is_sit_out(int position) const
{
    static const std::array<const char*, 2> ids = { "sitout-0", "sitout-1" };

    if (const auto p = settings_->get_pixel(ids[position]))
        return window_->is_pixel(*p);
    else if (const auto p = settings_->get_regex("stack-sitout"))
        return std::regex_match(get_stack_text(position), *p);
    else
        return false;
}

double table_manager::get_pot() const
{
    if (const auto p = settings_->get_label("pot"))
    {
        const auto s = read_label(*window_, mono_image_.get(), *settings_, *p);
        return s.empty() ? 0 : std::stof(s);
    }
    else
        return 0;
}

bool table_manager::is_highlight(int position) const
{
    static const std::array<const char*, 2> ids = { "active-0", "active-1" };

    return window_->is_pixel(*settings_->get_pixel(ids[position]));
}

bool table_manager::is_captcha() const
{
    if (const auto p = settings_->get_pixel("captcha"))
        return window_->is_pixel(*p);
    else
        return false;
}

void table_manager::input_captcha(const std::string& str) const
{
    const auto& button = settings_->get_button("captcha");

    if (button && window_->click_button(input_, *button))
    {
        input_.send_string(str);
        input_.sleep();
    }
}

std::string table_manager::snapshot_t::to_string(const snapshot_t& snapshot)
{
    std::stringstream ss;

    ss << "board: ["
       << get_card_string(snapshot.board[0]) << ", "
       << get_card_string(snapshot.board[1]) << ", "
       << get_card_string(snapshot.board[2]) << ", "
       << get_card_string(snapshot.board[3]) << ", "
       << get_card_string(snapshot.board[4]) << "]\n";

    ss << "hole: [" << get_card_string(snapshot.hole[0]) << ", " << get_card_string(snapshot.hole[1]) << "]\n";
    ss << "dealer: [" << snapshot.dealer[0] << ", " << snapshot.dealer[1] << "]\n";
    ss << "stack: [" << snapshot.stack[0] << ", " << snapshot.stack[1] << "]\n";
    ss << "bet: [" << snapshot.bet[0] << ", " << snapshot.bet[1] << "]\n";
    ss << "total_pot: " << snapshot.total_pot << "\n";
    ss << "all_in: [" << static_cast<int>(snapshot.all_in[0]) << ", "
        << static_cast<int>(snapshot.all_in[1]) << "]\n";
    ss << "buttons: " << snapshot.buttons << "\n";
    ss << "sit_out: [" << static_cast<int>(snapshot.sit_out[0]) << ", "
        << static_cast<int>(snapshot.sit_out[1]) << "]\n";
    ss << "highlight: [" << static_cast<int>(snapshot.highlight[0]) << ", "
        << static_cast<int>(snapshot.highlight[1]) << "]\n";
    ss << "captcha: " << static_cast<int>(snapshot.captcha) << "\n";

    return ss.str();
}
