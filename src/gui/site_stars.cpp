#include "site_stars.h"

#pragma warning(push, 3)
#include <QImage>
#include <QPixmap>
#define NOMINMAX
#include <Windows.h>
#pragma warning(pop)

#include "util/card.h"
#include "abslib/holdem_abstraction.h"
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
        if (image.pixel(35, 273) != qRgb(194, 194, 194))
            return 0;
        else if (image.pixel(670, 273) != qRgb(194, 194, 194))
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
            case 0x7f8: c = '0'; x += 6; break;
            case 0x030: c = '1'; x += 4; break;
            case 0xc08: c = '2'; x += 6; break;
            case 0x408: c = '3'; x += 6; break;
            case 0x300: c = '4'; x += 6; break;
            case 0x470: c = '5'; x += 6; break;
            case 0x7f0: c = '6'; x += 6; break;
            case 0x004: c = '7'; x += 6; break;
            case 0x7b8: c = '8'; x += 6; break;
            case 0x478:
                switch (read_column(image, ++x, rect.top(), rect.top() + rect.height(), color))
                {
                case 0xcfc: c = '9'; x += 5; break;
                default: x += 4; break; // $
                }
                break;
            case 0xc00: c = '.'; x += 2; break;
            case 0xffc: x += 23; break; // Pot:
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
        const QRect left_rect(36, 273, 91, 15);
        const QRect right_rect(671, 273, 91, 15);
        const QRect rect = player == 0 ? left_rect : right_rect;
        const bool flashing = (image.pixel(QPoint(rect.left() - 1, rect.top())) != qRgb(194, 194, 194));
        const QRgb color = flashing ? qRgb(32, 32, 32) : qRgb(192, 192, 192);
        return std::atof(read_text_type1(image, rect, color).c_str());
    }

    int get_dealer(const QImage& image)
    {
        if (image.pixel(QPoint(144, 269)) == qRgb(162, 97, 33))
            return 0;
        else if (image.pixel(QPoint(645, 269)) == qRgb(162, 97, 33))
            return 1;
        else
            return -1;
    }

    int get_board_card(const QImage& image, const QRect& rect)
    {
        const std::array<QRgb, 4> colors = {{
            qRgb(41, 133, 10), // clubs
            qRgb(10, 10, 238), // diamonds
            qRgb(202, 18, 18), // hearts
            qRgb(2, 2, 2), // spades
        }};

        for (int x = rect.left(); x < rect.right(); ++x)
        {
            for (int suit = 0; suit < 4; ++suit)
            {
                const QRgb& color = colors[suit];
                int rank = -1;

                switch (read_column(image, x, rect.top(), rect.top() + rect.height(), color))
                {
                case 0x1000: rank = 0; break; // 2
                case 0x0400: rank = 1; break; // 3
                case 0x0100: rank = 2; break; // 4
                case 0x0040: rank = 3; break; // 5
                case 0x00e0: rank = 4; break; // 6
                case 0x000f: rank = 5; break; // 7
                case 0x0300:
                    switch (read_column(image, ++x, rect.top(), rect.top() + rect.height(), color))
                    {
                    case 0x0f88: rank = 6; break; // 8
                    case 0x0f00: rank = 9; break; // J
                    }
                    break;
                case 0x0010: rank = 7; break; // 9
                case 0x1ffe: rank = 8; break; // 10
                case 0x0180: rank = 10; break; // Q
                case 0x1803: rank = 11; break; // K
                case 0x1800: rank = 12; break; // A
                }

                if (rank != -1)
                    return get_card(rank, suit);
            }
        }

        return -1;
    }

    double get_bet_size(const QImage& image, const QRect& rect)
    {
        const QRgb color = qRgb(255, 246, 207);
        std::string s;

        for (int x = rect.left(); x < rect.right(); )
        {
            char c = 0;

            switch (read_column(image, x, rect.top(), rect.top() + rect.height(), color))
            {
            case 0x0fc:
                switch (read_column(image, x += 2, rect.top(), rect.top() + rect.height(), color))
                {
                case 0x102: c = '0'; x += 3; break;
                default: c = '6'; x += 3; break;
                }
                break;
            case 0x018: c = '1'; x += 4; break;
            case 0x184: c = '2'; x += 5; break;
            case 0x084: c = '3'; x += 5; break;
            case 0x060: c = '4'; x += 6; break;
            case 0x0b8: c = '5'; x += 5; break;
            case 0x002: c = '7'; x += 5; break;
            case 0x0ec: c = '8'; x += 5; break;
            case 0x09c:
                switch (read_column(image, ++x, rect.top(), rect.top() + rect.height(), color))
                {
                case 0x1be: c = '9'; x += 4; break;
                case 0x132: x += 4; break; // $
                }
                break;
            case 0x180: c = '.'; x += 2; break;
            }

            if (c)
                s += c;
            else
                ++x;
        }

        return std::atof(s.c_str());
    }

    double get_total_pot(const QImage& image)
    {
        return std::atof(read_text_type1(image, QRect(336, 11, 124, 15), qRgb(0, 0, 0)).c_str());
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

site_stars::site_stars(WId window)
    : dealer_(-1)
    , player_(-1)
    , round_(-1)
    , big_blind_(-1)
    , action_(-1)
    , stack_bb_(-1)
    , new_hand_(true)
    , fraction_(-1)
    , window_(window)
{
    hole_.fill(-1);
    board_.fill(-1);
}

bool site_stars::update()
{
    if (!IsWindow(window_))
    {
        dealer_ = -1;
        return false;
    }

    const auto title = window_manager::get_window_text(window_);
    const std::regex re(".*\\$[0-9.]+/\\$([0-9.]+).*");
    std::smatch match;

    if (std::regex_match(title, match, re))
        big_blind_ = std::atof(match[1].str().c_str());

    const auto image = screenshot(window_).toImage();

    if (image.isNull())
        return false;

    const int dealer = ::get_dealer(image);
    const int player = get_active_player(image);

    if (dealer == -1 || player == -1)
        return false;

    new_hand_ = dealer != dealer_;
    dealer_ = dealer;
    const bool new_player = player != player_;
    player_ = player;

    const double stack = ::get_stack_size(image, 0);
    const double stack2 = ::get_stack_size(image, 1);
    const double total_pot = get_total_pot(image);

    hole_[0] = get_board_card(image, QRect(51, 112, 50, 13));
    hole_[1] = get_board_card(image, QRect(66, 116, 50, 13));
    board_[0] = get_board_card(image, QRect(268, 157, 50, 13));
    board_[1] = get_board_card(image, QRect(322, 157, 50, 13));
    board_[2] = get_board_card(image, QRect(376, 157, 50, 13));
    board_[3] = get_board_card(image, QRect(430, 157, 50, 13));
    board_[4] = get_board_card(image, QRect(484, 157, 50, 13));

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

    const double left_bet = get_bet_size(image, QRect(128, 235, 140, 10));
    const double right_bet = get_bet_size(image, QRect(534, 235, 136, 10));
    
    if (new_hand_ && big_blind_ > 0)
        stack_bb_ = std::min(stack, stack2) / big_blind_;

    const double last_bet = player_ == 0 ? right_bet : left_bet;
    const double to_call = last_bet - (player_ == 0 ? left_bet : right_bet);
 
    if (stack == 0 || stack2 == 0)
    {
        action_ = RAISE;
        fraction_ = std::numeric_limits<double>::max();
    }
    else if ((round_ != holdem_abstraction::PREFLOP && (new_round || (new_player && last_bet == 0))) || (left_bet > 0 && left_bet == right_bet))
    {
        action_ = CALL;
    }
    else if (big_blind_ > 0
        && ((round_ == holdem_abstraction::PREFLOP && (left_bet > big_blind_ || right_bet > big_blind_)
        || (round_ != holdem_abstraction::PREFLOP && (left_bet >= big_blind_ || right_bet >= big_blind_)))))
    {
        action_ = RAISE;
        fraction_ = to_call / total_pot;
    }
    else
    {
        action_ = -1;
    }

    return true;
}

int site_stars::get_action() const
{
    return action_;
}

std::pair<int, int> site_stars::get_hole_cards() const
{
    return std::make_pair(hole_[0], hole_[1]);
}

void site_stars::get_board_cards(std::array<int, 5>& board) const
{
    board = board_;
}

bool site_stars::is_new_hand() const
{
    return new_hand_;
}

int site_stars::get_round() const
{
    return round_;
}

double site_stars::get_raise_fraction() const
{
    return fraction_;
}

int site_stars::get_dealer() const
{
    return dealer_;
}

int site_stars::get_stack_size() const
{
    return int(stack_bb_ * 2 + 0.5);
}

bool site_stars::is_action_needed() const
{
    // TODO implement
    return false;
}

void site_stars::fold() const
{
    // TODO implement
}

void site_stars::call() const
{
    // TODO implement
}

void site_stars::raise(double) const
{
    // TODO implement
}
