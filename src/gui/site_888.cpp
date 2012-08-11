#include "site_888.h"

#pragma warning(push, 3)
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <QImage>
#include <QPixmap>
#define NOMINMAX
#include <Windows.h>
#pragma warning(pop)

#include "util/card.h"
#include "abslib/holdem_abstraction.h"
#include "input_manager.h"
#include "window_manager.h"

namespace
{
    int read_column(const QImage& image, const int x, const int top, const int bottom, const QRgb& color)
    {
        int bitmap = 0;

        for (int y = top; y < bottom; ++y)
        {
            const int bit = image.pixel(x, y) == color;
            bitmap |= bit << (y - top);
        }

        return bitmap;
    }

    int get_active_player(const QImage& image)
    {
        if (image.pixel(310, 365) == qRgb(255, 255, 255))
            return 0;
        else if (image.pixel(310, 88) == qRgb(255, 255, 255))
            return 1;
        else
            return -1;
    }

    std::string read_text_type1(const QImage& image, const QRect& rect, const QRgb& color)
    {
        std::string s;

        for (int x = rect.left(); x < rect.right(); )
        {
            char c = 0;

            switch (read_column(image, x, rect.top(), rect.top() + rect.height(), color))
            {
            case 0x1fc:
                switch (read_column(image, ++x, rect.top(), rect.top() + rect.height(), color))
                {
                case 0x306: c = '0'; x += 4; break;
                case 0x314: c = '6'; x += 5; break;
                }
                break;
            case 0x204: c = '1'; x += 5; break;
            case 0x306: c = '2'; x += 6; break;
            case 0x100: c = '3'; x += 6; break;
            case 0x060: c = '4'; x += 6; break;
            case 0x31e: c = '5'; x += 6; break;
            case 0x002:
                switch (read_column(image, ++x, rect.top(), rect.top() + rect.height(), color))
                {
                case 0x202: c = '7'; x += 5; break;
                case 0x002: x += 49 - 1; break; // Total Pot
                }
                break;
            case 0x080: c = '8'; x += 7; break;
            case 0x018: c = '9'; x += 7; break;
            case 0x338: x += 6 - 1; break; // $
            case 0x300: c = '.'; x += 2; break;
            case 0x1f0: c = 'c'; x += 5; break;
            }

            if (c)
                s += c;
            else
                ++x;
        }

        return s;
    }

    std::string read_text_type2(const QImage& image, const QRect& rect, const QRgb& color)
    {
        std::string s;

        for (int x = rect.left(); x < rect.right(); )
        {
            char c = 0;

            switch (read_column(image, x, rect.top(), rect.top() + rect.height(), color))
            {
            case 0xfe: c = '0'; x += 7; break;
            case 0x102: c = '1'; x += 5; break;
            case 0x103: c = '2'; x += 7; break;
            case 0x183: c = '3'; x += 6; break;
            case 0x20: c = '4'; x += 8; break;
            case 0x18f: c = '5'; x += 6; break;
            case 0xfc: c = '6'; x += 7; break;
            case 0x101: c = '7'; x += 7; break;
            case 0xee: c = '8'; x += 7; break;
            case 0x11e: c = '9'; x += 6; break;
            case 0x11c: x += 7 - 1; break; // $
            case 0x180: c = '.'; x += 2; break;
            }

            if (c)
                s += c;
            else
                ++x;
        }

        return s;
    }

    std::string read_text_type3(const QImage& image, const QRect& rect, const QRgb& color)
    {
        std::string s;

        for (int x = rect.left(); x < rect.right(); )
        {
            char c = 0;

            switch (read_column(image, x, rect.top(), rect.top() + rect.height(), color))
            {
            case 0xfe: c = '0'; x += 7; break;
            case 0x102: c = '1'; x += 7; break;
            case 0x103: c = '2'; x += 7; break;
            case 0x183: c = '3'; x += 7; break;
            case 0x20: c = '4'; x += 8; break;
            case 0x18f: c = '5'; x += 7; break;
            case 0xfc: c = '6'; x += 7; break;
            case 0x101: c = '7'; x += 7; break;
            case 0xee: c = '8'; x += 7; break;
            case 0x11e: c = '9'; x += 7; break;
            case 0x19c: x += 7 - 1; break; // $
            case 0x180: c = '.'; x += 3; break;
            }

            if (c)
                s += c;
            else
                ++x;
        }

        return s;
    }

    double get_stack_size(const QImage& image, const int player)
    {
        const QRect left_rect(312, 390, 150, 11);
        const QRect right_rect(312, 68, 150, 11);
        const QRect rect = player == 0 ? left_rect : right_rect;
        const bool flashing = (image.pixel(QPoint(rect.left() - 1, rect.top())) == qRgb(255, 255, 255));
        const auto s = flashing ? read_text_type3(image, rect, qRgb(255, 255, 255))
            : read_text_type2(image, rect, qRgb(255, 255, 255));
        return s.empty() ? 0 : std::atof(s.c_str());
    }

    int get_dealer(const QImage& image)
    {
        if (image.pixel(QPoint(332, 300)) == qRgb(255, 255, 255))
            return 0;
        else if (image.pixel(QPoint(332, 99)) == qRgb(255, 255, 255))
            return 1;
        else
            return -1;
    }

    int get_board_card(const QImage& image, const QRect& rect)
    {
        const std::array<QRgb, 4> colors = {{
            qRgb(29, 149, 5), // clubs
            qRgb(17, 27, 151), // diamonds
            qRgb(255, 0, 0), // hearts
            qRgb(0, 0, 0), // spades
        }};

        for (int x = rect.left(); x < rect.right(); ++x)
        {
            for (int suit = 0; suit < 4; ++suit)
            {
                const QRgb& color = colors[suit];
                int rank = -1;

                switch (read_column(image, x, rect.top(), rect.top() + rect.height(), color))
                {
                case 0x18010: rank = 0; break; // 2
                case 0x03008: rank = 1; break; // 3
                case 0x03c00: rank = 2; break; // 4
                case 0x01000: rank = 3; break; // 5
                case 0x01fe0: rank = 4; break; // 6
                case 0x00007: rank = 5; break; // 7
                case 0x03830: rank = 6; break; // 8
                case 0x03838: rank = 6; break; // 8
                case 0x000f8: rank = 7; break; // 9
                case 0x020f8: rank = 7; break; // 9
                case 0x00010: rank = 8; break; // 10
                case 0x03000:
                    switch (read_column(image, ++x, rect.top(), rect.top() + rect.height(), color))
                    {
                    case 0x981a: rank = 1; break; // 3h
                    case 0x4000: rank = 9; break; // Jh
                    }
                    break;
                case 0x07800: rank = 9; break; // J
                case 0x00380: rank = 10; break; // Q
                case 0x1ffff: rank = 11; break; // K
                case 0x1e000: rank = 12; break; // A
                }

                if (rank != -1)
                    return get_card(rank, suit);
            }
        }

        return -1;
    }

    double read_bet_size(const QImage& image, const QRect& rect)
    {
        const auto s = read_text_type1(image, rect, qRgb(255, 255, 255));
        double d = std::atof(s.c_str());
        
        if (!s.empty() && s[s.length() - 1] == 'c')
            d /= 100;

        return d;
    }

    enum action { NONE, FOLD, CALL, RAISE, ALLIN };

    action get_last_action(const QImage& image, int player)
    {
        const QPoint p = player == 0 ? QPoint(354, 405) : QPoint(354, 83);

        if (image.pixel(p) == qRgb(5, 130, 214))
            return FOLD;
        else if (image.pixel(p) == qRgb(102, 190, 44))
            return CALL;
        else if (image.pixel(p) == qRgb(254, 127, 3))
            return RAISE;
        else if (image.pixel(p) == qRgb(255, 0, 0))
            return ALLIN;
        else
            return NONE;
    }

    QPixmap screenshot(WId winId)
    {
        RECT r;
        GetClientRect(winId, &r);

        const int w = r.right - r.left;
        const int h = r.bottom - r.top;

        const HDC display_dc = GetDC(0);
        const HDC bitmap_dc = CreateCompatibleDC(display_dc);
        const HBITMAP bitmap = CreateCompatibleBitmap(display_dc, w, h);
        const HGDIOBJ null_bitmap = SelectObject(bitmap_dc, bitmap);

        const HDC window_dc = GetDC(winId);
        BitBlt(bitmap_dc, 0, 0, w, h, window_dc, 0, 0, SRCCOPY);

        ReleaseDC(winId, window_dc);
        SelectObject(bitmap_dc, null_bitmap);
        DeleteDC(bitmap_dc);

        const QPixmap pixmap = QPixmap::fromWinHBITMAP(bitmap);

        DeleteObject(bitmap);
        ReleaseDC(0, display_dc);

        return pixmap;
    }
}

site_888::site_888(input_manager& input_manager, WId window)
    : dealer_(-1)
    , player_(-1)
    , round_(-1)
    , big_blind_(-1)
    , action_(-1)
    , stack_bb_(-1)
    , new_hand_(true)
    , fraction_(-1)
    , window_(window)
    , input_(input_manager)
    , big_blind_regex_(".*\\$[0-9.]+/\\$([0-9.]+).*")
{
    hole_.fill(-1);
    board_.fill(-1);
}

bool site_888::update()
{
    if (!IsWindow(window_))
    {
        dealer_ = -1;
        return false;
    }

    const auto image = screenshot(window_).toImage();

    if (image.isNull())
        return false;

    const auto mono_image = image.convertToFormat(QImage::Format_Mono, Qt::ThresholdDither);

    //image.save("test.png");
    //mono_image.save("testmono.png");

    const int dealer = ::get_dealer(image);
    const int player = get_active_player(image);

    hole_[0] = get_board_card(image, QRect(338, 326, 48, 17));
    hole_[1] = get_board_card(image, QRect(388, 326, 48, 17));
    board_[0] = get_board_card(image, QRect(264, 156, 48, 17));
    board_[1] = get_board_card(image, QRect(318, 156, 48, 17));
    board_[2] = get_board_card(image, QRect(372, 156, 48, 17));
    board_[3] = get_board_card(image, QRect(426, 156, 48, 17));
    board_[4] = get_board_card(image, QRect(480, 156, 48, 17));

    can_raise_ = image.pixel(639, 437) == qRgb(0, 0, 0);
    action_needed_ = player == 0 ? image.pixel(327, 437) == qRgb(0, 0, 0) : false;

    total_pot_ = read_bet_size(mono_image, QRect(263, 140, 266, 12));
    bets_[0] = read_bet_size(mono_image, QRect(263, 310, 266, 12));
    bets_[1] = read_bet_size(mono_image, QRect(263, 124, 266, 12));

    const auto title = window_manager::get_window_text(window_);
    std::smatch match;

    if (std::regex_match(title, match, big_blind_regex_))
        big_blind_ = std::atof(match[1].str().c_str());

    if (dealer == -1 || player == -1)
        return false;

    std::array<double, 2> stack = {::get_stack_size(mono_image, 0), ::get_stack_size(mono_image, 1)};

    // TODO this might sometime result in a new hand not being detected
    if (dealer != dealer_ && (stack[0] == 0 || stack[1] == 0))
        return false;

    new_hand_ = dealer != dealer_;
    dealer_ = dealer;
    const bool new_player = player != player_;
    player_ = player;    

    int round;

    if (board_[4] != -1)
        round = holdem_abstraction::RIVER;
    else if (board_[3] != -1)
        round = holdem_abstraction::TURN;
    else if (board_[0] != -1)
        round = holdem_abstraction::FLOP;
    else
        round = holdem_abstraction::PREFLOP;

    const bool new_round = round != round_;
    round_ = round;

    if (!new_player && !new_round && !new_hand_)
        return false;

    if (new_hand_ && big_blind_ > 0 && stack[0] > 0 && stack[1] > 0)
        stack_bb_ = std::min(stack[0] + bets_[0], stack[1] + bets_[1]) / big_blind_;

    const double last_bet = player_ == 0 ? bets_[1] : bets_[0];
    to_call_ = last_bet - (player_ == 0 ? bets_[0] : bets_[1]);
 
    if (get_last_action(image, 0) == ALLIN || get_last_action(image, 1) == ALLIN)
    {
        action_ = RAISE;
        fraction_ = std::numeric_limits<double>::max();
    }
    else if (get_last_action(image, 0) == CALL || (round_ != holdem_abstraction::PREFLOP
        && (new_round || (new_player && last_bet == 0))) || (bets_[0] > 0 && bets_[0] == bets_[1]))
    {
        action_ = CALL;
    }
    else if (get_last_action(image, 0) == RAISE || (big_blind_ > 0
        && ((round_ == holdem_abstraction::PREFLOP && (bets_[0] > big_blind_ || bets_[1] > big_blind_)
        || (round_ != holdem_abstraction::PREFLOP && (bets_[0] >= big_blind_ || bets_[1] >= big_blind_))))))
    {
        action_ = RAISE;
        fraction_ = to_call_ / total_pot_;
    }
    else
    {
        action_ = -1;
    }

    return true;
}

int site_888::get_action() const
{
    return action_;
}

std::pair<int, int> site_888::get_hole_cards() const
{
    return std::make_pair(hole_[0], hole_[1]);
}

void site_888::get_board_cards(std::array<int, 5>& board) const
{
    board = board_;
}

bool site_888::is_new_hand() const
{
    return new_hand_;
}

int site_888::get_round() const
{
    return round_;
}

double site_888::get_raise_fraction() const
{
    return fraction_;
}

int site_888::get_dealer() const
{
    return dealer_;
}

int site_888::get_stack_size() const
{
    return int(stack_bb_ * 2 + 0.5);
}

bool site_888::is_action_needed() const
{
    return action_needed_;
}

void site_888::fold() const
{
    input_.send_keypress(VK_F5);
}

void site_888::call() const
{
    input_.send_keypress(VK_F6);
}

void site_888::raise(double fraction) const
{
    if (can_raise_)
    {
        const double amount = int((fraction * (total_pot_ + to_call_) + to_call_ + bets_[0]) / big_blind_) * big_blind_;
        input_.send_string(boost::lexical_cast<std::string>(amount));
        input_.sleep();
        input_.send_keypress(VK_F7);
    }
    else
    {
        call();
    }
}
