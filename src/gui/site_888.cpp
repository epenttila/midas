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

    int read_card(const QImage& image, const QRect& rect)
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
    : window_(window)
    , input_(input_manager)
{
}

void site_888::update()
{
    if (!IsWindow(window_))
    {
        image_.reset();
        mono_image_.reset();
        return;
    }

    if (!image_)
        image_.reset(new QImage);

    *image_ = screenshot(window_).toImage();

    if (image_->isNull())
    {
        image_.reset();
        mono_image_.reset();
        return;
    }

    if (!mono_image_)
        mono_image_.reset(new QImage);

    *mono_image_ = image_->convertToFormat(QImage::Format_Mono, Qt::ThresholdDither);
}

std::pair<int, int> site_888::get_hole_cards() const
{
    if (!image_)
        return std::make_pair(-1, -1);

    return std::make_pair(read_card(*image_, QRect(338, 326, 48, 17)), read_card(*image_, QRect(388, 326, 48, 17)));
}

void site_888::get_board_cards(std::array<int, 5>& board) const
{
    if (!image_)
    {
        board.fill(-1);
        return;
    }

    board[0] = read_card(*image_, QRect(264, 156, 48, 17));
    board[1] = read_card(*image_, QRect(318, 156, 48, 17));
    board[2] = read_card(*image_, QRect(372, 156, 48, 17));
    board[3] = read_card(*image_, QRect(426, 156, 48, 17));
    board[4] = read_card(*image_, QRect(480, 156, 48, 17));
}

int site_888::get_dealer() const
{
    if (!image_)
        return -1;

    if (image_->pixel(QPoint(332, 300)) == qRgb(255, 255, 255))
        return 0;
    else if (image_->pixel(QPoint(332, 99)) == qRgb(255, 255, 255))
        return 1;
    else
        return -1;
}

void site_888::fold() const
{
    input_.send_keypress(VK_F5);
}

void site_888::call() const
{
    input_.send_keypress(VK_F6);
}

void site_888::raise(double amount) const
{
    if (get_buttons() & RAISE_BUTTON)
    {
        input_.send_string(boost::lexical_cast<std::string>(amount));
        input_.sleep();
        input_.send_keypress(VK_F7);
    }
    else
    {
        call();
    }
}

double site_888::get_stack(int player) const
{
    if (!mono_image_)
        return -1;

    const QRect left_rect(353, 390, 150, 11);
    const QRect right_rect(353, 68, 150, 11);
    const QRect rect = player == 0 ? left_rect : right_rect;
    const bool flashing = (mono_image_->pixel(QPoint(rect.left() - 43, rect.top())) == qRgb(255, 255, 255));
    const auto s = flashing ? read_text_type3(*mono_image_, rect, qRgb(255, 255, 255))
        : read_text_type2(*mono_image_, rect, qRgb(255, 255, 255));
    return s.empty() ? 0 : std::atof(s.c_str());
}

double site_888::get_bet(int player) const
{
    if (!mono_image_)
        return -1;

    return player == 0 ? read_bet_size(*mono_image_, QRect(263, 310, 266, 12))
        : read_bet_size(*mono_image_, QRect(263, 124, 266, 12));
}

double site_888::get_big_blind() const
{
    const auto title = window_manager::get_window_text(window_);
    const std::regex re(".*\\$?[0-9.]+[^/]*/(\\$?)([0-9.]+).*");
    std::smatch match;

    double big_blind = -1;

    if (std::regex_match(title, match, re))
    {
        big_blind = std::atof(match[2].str().c_str());

        if (match.length(1) == 0)
            big_blind /= 100;
    }

    return big_blind;
}

int site_888::get_player() const
{
    if (!image_)
        return -1;

    if (image_->pixel(310, 365) == qRgb(255, 255, 255))
        return 0;
    else if (image_->pixel(310, 88) == qRgb(255, 255, 255))
        return 1;
    else
        return -1;
}

double site_888::get_total_pot() const
{
    return mono_image_ ? read_bet_size(*mono_image_, QRect(263, 140, 266, 12)) : -1;
}

bool site_888::is_opponent_allin() const
{
    return image_ ? image_->pixel(374, 67) == qRgb(255, 0, 0) : false;
}

int site_888::get_buttons() const
{
    if (!image_)
        return 0;

    int buttons = 0;

    if (image_->pixel(328, 469) == qRgb(5, 130, 213) || image_->pixel(328, 469) == qRgb(18, 162, 226))
        buttons |= FOLD_BUTTON;

    if (image_->pixel(484, 469) == qRgb(89, 218, 0) || image_->pixel(484, 469) == qRgb(126, 229, 0))
        buttons |= CALL_BUTTON;

    if (image_->pixel(640, 469) == qRgb(255, 130, 7) || image_->pixel(640, 469) == qRgb(255, 162, 23)
        || image_->pixel(640, 469) == qRgb(255, 0, 0))
    {
        buttons |= RAISE_BUTTON;
    }

    return buttons;
}

bool site_888::is_opponent_sitout() const
{
    return image_ ? image_->pixel(367, 67) == qRgb(113, 113, 113) : false;
}
