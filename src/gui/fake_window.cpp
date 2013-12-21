#include "fake_window.h"

#pragma warning(push, 1)
#include <cassert>
#include <QDateTime>
#include <Windows.h>
#pragma warning(pop)

#include "window_utils.h"
#include "input_manager.h"

Q_GUI_EXPORT QPixmap qt_pixmapFromWinHBITMAP(HBITMAP, int hbitmapFormat = 0);

namespace
{
    QPixmap screenshot(WId window, const QRect& rect)
    {
        const auto hwnd = reinterpret_cast<HWND>(window);
        const HDC display_dc = GetDC(0);
        const HDC bitmap_dc = CreateCompatibleDC(display_dc);
        const HBITMAP bitmap = CreateCompatibleBitmap(display_dc, rect.width(), rect.height());
        const HGDIOBJ null_bitmap = SelectObject(bitmap_dc, bitmap);

        const HDC window_dc = GetDC(hwnd);
        BitBlt(bitmap_dc, 0, 0, rect.width(), rect.height(), window_dc, rect.left(), rect.top(), SRCCOPY);
        ReleaseDC(hwnd, window_dc);

        SelectObject(bitmap_dc, null_bitmap);
        DeleteDC(bitmap_dc);

        const QPixmap pixmap = qt_pixmapFromWinHBITMAP(bitmap);

        DeleteObject(bitmap);
        ReleaseDC(0, display_dc);

        return pixmap;
    }

    window_utils::font_data create_title_font()
    {
        window_utils::font_data font;

        font.children[0x0001].masks[0x0001] = std::make_pair("T", 5);
        font.children[0x0001].masks[0x01c1] = std::make_pair("7", 5);

        font.masks[0x0002] = std::make_pair("1", 4);

        font.children[0x0003].masks[0x0007] = std::make_pair("Y", 7);
        font.children[0x0003].children[0x001f].children[0x007c].children[0x03e0].children[0x03e0].children[0x007c].masks[0x001f] = std::make_pair("V", 1);
        font.children[0x0003].children[0x001f].children[0x007c].children[0x03e0].children[0x03e0].children[0x007c].masks[0x007c] = std::make_pair("W", 6);

        font.masks[0x0007] = std::make_pair("'", 2);

        font.masks[0x0018] = std::make_pair("v", 6);
        font.masks[0x0020] = std::make_pair("-", 3);
        font.masks[0x0044] = std::make_pair("#", 7);
        font.masks[0x0060] = std::make_pair("4", 6);
        font.masks[0x0078] = std::make_pair("w", 8);
        font.masks[0x0082] = std::make_pair("3", 6);
        font.masks[0x008c] = std::make_pair("$", 6);

        font.children[0x008e].children[0x019f].children[0x0111].children[0x0111].masks[0x01f3] = std::make_pair("S", 2);
        font.children[0x008e].children[0x019f].children[0x0111].children[0x0111].masks[0x01ff] = std::make_pair("9", 2);

        font.masks[0x0090] = std::make_pair("s", 5);
        font.masks[0x009f] = std::make_pair("5", 6);

        font.children[0x00c0].masks[0x01c0] = std::make_pair("J", 4);
        font.children[0x00c0].masks[0x01e8] = std::make_pair("a", 5);

        font.masks[0x00e6] = std::make_pair("&", 6);
        font.masks[0x00ee] = std::make_pair("8", 6);

        font.children[0x00f0].children[0x01f8].children[0x0108].children[0x0108].masks[0x0198] = std::make_pair("c", 2);
        font.children[0x00f0].children[0x01f8].children[0x0108].children[0x0108].masks[0x01f8] = std::make_pair("o", 2);
        font.children[0x00f0].children[0x01f8].children[0x0108].children[0x0108].masks[0x01ff] = std::make_pair("d", 2);
        font.children[0x00f0].children[0x01f8].children[0x0108].children[0x0108].masks[0x07f8] = std::make_pair("q", 2);
        font.children[0x00f0].children[0x01f8].masks[0x0128] = std::make_pair("e", 4);

        font.masks[0x00f8] = std::make_pair("u", 6);

        font.children[0x00fe].masks[0x01fe] = std::make_pair("t", 3);
        font.children[0x00fe].children[0x01ff].children[0x0101].children[0x0101].children[0x0101].masks[0x0183] = std::make_pair("C", 2);
        font.children[0x00fe].children[0x01ff].children[0x0101].children[0x0101].children[0x0101].masks[0x01ff] = std::make_pair("O", 2);
        font.children[0x00fe].children[0x01ff].children[0x0101].children[0x0101].masks[0x01ff] = std::make_pair("0", 2);
        font.children[0x00fe].children[0x01ff].children[0x0101].masks[0x0111] = std::make_pair("G", 4);
        font.children[0x00fe].children[0x01ff].children[0x0101].masks[0x0141] = std::make_pair("Q", 4);
        font.children[0x00fe].children[0x01ff].masks[0x0111] = std::make_pair("6", 5);

        font.masks[0x00ff] = std::make_pair("U", 7);
        font.masks[0x0100] = std::make_pair(".", 2);
        font.masks[0x0108] = std::make_pair(":", 2);

        font.children[0x0180].masks[0x01e0] = std::make_pair("/", 5);
        font.children[0x0180].masks[0x01f0] = std::make_pair("A", 8);

        font.masks[0x0181] = std::make_pair("Z", 8);
        font.masks[0x0182] = std::make_pair("2", 6);
        font.masks[0x0183] = std::make_pair("X", 8);
        font.masks[0x0188] = std::make_pair("z", 5);
        font.masks[0x0198] = std::make_pair("x", 5);

        font.children[0x01f8].children[0x01f8].children[0x0008].masks[0x0000] = std::make_pair("r", 0);
        font.children[0x01f8].children[0x01f8].children[0x0008].masks[0x01f8] = std::make_pair("m", 5);
        font.children[0x01f8].children[0x01f8].masks[0x0018] = std::make_pair("n", 4);

        font.masks[0x01f9] = std::make_pair("i", 2);
        font.masks[0x01fe] = std::make_pair("f", 3);

        font.children[0x01ff].children[0x01ff].masks[0x0000] = std::make_pair("I", 0);
        font.children[0x01ff].children[0x01ff].masks[0x0000] = std::make_pair("l", 0);
        font.children[0x01ff].children[0x01ff].masks[0x0010] = std::make_pair("H", 5);
        font.children[0x01ff].children[0x01ff].children[0x0011].children[0x0011].children[0x0011].masks[0x0001] = std::make_pair("F", 1);
        font.children[0x01ff].children[0x01ff].children[0x0011].children[0x0011].children[0x0011].masks[0x001f] = std::make_pair("P", 2);
        font.children[0x01ff].children[0x01ff].children[0x0011].children[0x0011].children[0x0011].masks[0x01ff] = std::make_pair("R", 2);
        font.children[0x01ff].children[0x01ff].masks[0x0018] = std::make_pair("h", 4);
        font.children[0x01ff].children[0x01ff].masks[0x001e] = std::make_pair("N", 5);
        font.children[0x01ff].children[0x01ff].children[0x003c].masks[0x0066] = std::make_pair("K", 4);
        font.children[0x01ff].children[0x01ff].children[0x003c].masks[0x00f0] = std::make_pair("M", 5);
        font.children[0x01ff].children[0x01ff].masks[0x0070] = std::make_pair("k", 4);
        font.children[0x01ff].children[0x01ff].masks[0x0100] = std::make_pair("L", 4);
        font.children[0x01ff].children[0x01ff].children[0x0101].masks[0x0101] = std::make_pair("D", 4);
        font.children[0x01ff].children[0x01ff].masks[0x0108] = std::make_pair("b", 4);
        font.children[0x01ff].children[0x01ff].children[0x0111].children[0x0111].masks[0x0111] = std::make_pair("E", 2);
        font.children[0x01ff].children[0x01ff].children[0x0111].children[0x0111].masks[0x01ff] = std::make_pair("B", 2);

        font.masks[0x0200] = std::make_pair(",", 3);
        font.masks[0x0400] = std::make_pair("y", 6);
        font.masks[0x04f0] = std::make_pair("g", 6);
        font.masks[0x07f8] = std::make_pair("p", 6);
        font.masks[0x07f9] = std::make_pair("j", 2);

        return font;
    }
}

fake_window::fake_window(int id, const QRect& rect)
    : rect_(rect)
    , id_(id)
    , wid_(0)
    , border_(0)
    , title_font_(create_title_font())
{
}

bool fake_window::update(WId wid)
{
    if (wid != 0)
        wid_ = wid;

    if (!IsWindow(reinterpret_cast<HWND>(wid_)))
        return false;

    window_rect_ = rect_;

    const auto image = ::screenshot(wid_, window_rect_).toImage();

    border_ = 0;

    for (int x = 0; x < std::min(image.width(), image.height()); ++x)
    {
        if (image.pixel(x, x) == qRgb(0, 0, 0))
            break;

        ++border_;
    }

    int width = 0;
    int height = 0;

    if (border_ > 0)
    {
        for (int x = border_; x < image.width(); ++x)
        {
            if (image.pixel(x, border_) != qRgb(0, 0, 0))
                break;

            ++width;
        }

        width += 2 * border_;

        for (int y = border_; y < image.height(); ++y)
        {
            if (image.pixel(border_, y) != qRgb(0, 0, 0))
                break;

            ++height;
        }

        height += 2 * border_;

        border_ += 1; // client area inside black border
    }

    if (width > 0 && height > 0)
    {
        window_rect_.setWidth(width);
        window_rect_.setHeight(height);

        static const int WINDOW_BUTTONS_WIDTH = 60; // worst case: min, max, close
        static const int TITLE_HEIGHT = 20;
        static const int ICON_BORDER_X = 3;
        static const int ICON_BORDER_Y = 2;
        static const int ICON_WIDTH = 16;
        static const int ICON_HEIGHT = 16;

        bool icon = false;

        // image is in window_rect_ space
        for (int x = border_ + ICON_BORDER_X; x < border_ + ICON_BORDER_X + ICON_WIDTH; ++x)
        {
            for (int y = border_ + ICON_BORDER_Y; y < border_ + ICON_BORDER_Y + ICON_HEIGHT; ++y)
            {
                if (image.pixel(x, y) != qRgb(255, 255, 255)
                    && image.pixel(x, y) != qRgb(0, 128, 0)
                    && image.pixel(x, y) != qRgb(128, 0, 128))
                {
                    icon = true;
                    break;
                }
            }

            if (icon)
                break;
        }

        title_rect_ = QRect(window_rect_.left() + border_, window_rect_.top() + border_,
            window_rect_.width() - 2 * border_ - WINDOW_BUTTONS_WIDTH, TITLE_HEIGHT);

        if (icon)
            title_rect_.adjust(ICON_WIDTH + 2 * ICON_BORDER_X, 0, 0, 0);

        client_rect_ = window_rect_.adjusted(border_, border_ + title_rect_.height() + 1, -border_, -border_);
    }
    else
    {
        window_rect_ = QRect();
        client_rect_ = QRect();
    }

    if (window_rect_.isValid())
        window_image_ = ::screenshot(wid_, window_rect_).toImage();
    else
        window_image_ = QImage();

    if (client_rect_.isValid())
        client_image_ = ::screenshot(wid_, client_rect_).toImage();
    else
        client_image_ = QImage();

    return is_valid();
}

bool fake_window::is_valid() const
{
    if (!IsWindow(reinterpret_cast<HWND>(wid_)))
        return false;

    if (!window_rect_.isValid() || !client_rect_.isValid() || window_image_.isNull() || client_image_.isNull())
        return false;

    return window_image_.pixel(0, 0) == qRgb(255, 255, 255)
        && window_image_.pixel(window_rect_.width() - 1, 0) == qRgb(255, 255, 255)
        && window_image_.pixel(0, window_rect_.height() - 1) == qRgb(255, 255, 255)
        && window_image_.pixel(window_rect_.width() - 1, window_rect_.height() - 1) == qRgb(255, 255, 255);
}

bool fake_window::click_button(input_manager& input, const window_utils::button_data& button, bool double_click) const
{
    // check if button is there
    if (!window_utils::is_button(&client_image_, button))
        return false;

    const auto p = client_to_screen(QPoint(button.rect.x(), button.rect.y()));
    input.move_click(p.x(), p.y(), button.rect.width(), button.rect.height(), double_click);

    return true;
}

bool fake_window::click_any_button(input_manager& input, const std::vector<window_utils::button_data>& buttons) const
{
    for (int i = 0; i < buttons.size(); ++i)
    {
        if (click_button(input, buttons[i]))
            return true;
    }

    return false;
}

QPoint fake_window::client_to_screen(const QPoint& point) const
{
    auto tmp = client_rect_.topLeft() + point;
    POINT p = { tmp.x(), tmp.y() };
    ClientToScreen(reinterpret_cast<HWND>(wid_), &p);
    return QPoint(p.x, p.y);
}

int fake_window::get_id() const
{
    return id_;
}

std::string fake_window::get_window_text() const
{
    const auto rect = title_rect_.translated(-window_rect_.topLeft()).adjusted(0, 5, 0, -4);

    return window_utils::parse_image_text(&window_image_, rect, qRgb(255, 255, 255), title_font_);
}

QImage fake_window::get_window_image() const
{
    return window_image_;
}

QImage fake_window::get_client_image() const
{
    return client_image_;
}
