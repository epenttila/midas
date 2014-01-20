#include "fake_window.h"

#pragma warning(push, 1)
#include <Windows.h>
#pragma warning(pop)

#include "window_utils.h"
#include "input_manager.h"

Q_GUI_EXPORT QPixmap qt_pixmapFromWinHBITMAP(HBITMAP, int hbitmapFormat = 0);

namespace
{
    static const int WINDOW_BUTTONS_WIDTH = 50; // worst case: min, max, close
    static const int TITLE_HEIGHT = 18;
    static const int ICON_BORDER_X = 2;
    static const int ICON_BORDER_Y = 1;
    static const int ICON_WIDTH = 16;
    static const int ICON_HEIGHT = 16;
    static const int TITLE_OFFSET = 1;
    static const int TITLE_TEXT_LEFT_OFFSET = 0;
    static const int TITLE_TEXT_TOP_OFFSET = 2;
    static const int TITLE_TEXT_RIGHT_OFFSET = 0;
    static const int TITLE_TEXT_BOTTOM_OFFSET = -3;

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

    bool is_top_left_corner(const QImage& image, const int x, const int y)
    {
        return image.pixel(x,     y) == qRgb(212, 208, 200)
            && image.pixel(x + 1, y) == qRgb(212, 208, 200)
            && image.pixel(x + 2, y) == qRgb(212, 208, 200)
            && image.pixel(x,     y + 1) == qRgb(212, 208, 200)
            && image.pixel(x + 1, y + 1) == qRgb(255, 255, 255)
            && image.pixel(x + 2, y + 1) == qRgb(255, 255, 255)
            && image.pixel(x,     y + 2) == qRgb(212, 208, 200)
            && image.pixel(x + 1, y + 2) == qRgb(255, 255, 255)
            && image.pixel(x + 2, y + 2) == qRgb(212, 208, 200);
    }

    bool is_bottom_right_corner(const QImage& image, const int x, const int y)
    {
        return image.pixel(x,     y) == qRgb(212, 208, 200)
            && image.pixel(x + 1, y) == qRgb(128, 128, 128)
            && image.pixel(x + 2, y) == qRgb(64, 64, 64)
            && image.pixel(x,     y + 1) == qRgb(128, 128, 128)
            && image.pixel(x + 1, y + 1) == qRgb(128, 128, 128)
            && image.pixel(x + 2, y + 1) == qRgb(64, 64, 64)
            && image.pixel(x,     y + 2) == qRgb(64, 64, 64)
            && image.pixel(x + 1, y + 2) == qRgb(64, 64, 64)
            && image.pixel(x + 2, y + 2) == qRgb(64, 64, 64);
    }

    void try_top_left(const QImage& image, int* border, int* width, int* height)
    {
        if (is_top_left_corner(image, 0, 0))
        {
            if (image.pixel(3, 3) == qRgb(212, 208, 200))
                *border = 4;
            else
                *border = 3;
        }
        else
            return;

        // top edge
        for (int x = *border; x < image.width(); ++x)
        {
            if (image.pixel(x, 0) == qRgb(64, 64, 64)
                && image.pixel(x - 1, 1) == qRgb(128, 128, 128)
                && image.pixel(x - 2, 2) == qRgb(212, 208, 200))
            {
                *width = x + 1;
                break;
            }
        }

        // left edge
        for (int y = *border; y < image.height(); ++y)
        {
            if (image.pixel(0, y) == qRgb(64, 64, 64)
                && image.pixel(1, y - 1) == qRgb(128, 128, 128)
                && image.pixel(2, y - 2) == qRgb(212, 208, 200))
            {
                *height = y + 1;
                break;
            }
        }

        if (*width == -1 && *height != -1)
        {
            // bottom edge
            for (int x = *border; x < image.width(); ++x)
            {
                if (is_bottom_right_corner(image, x - 2, *height - 3))
                {
                    *width = x + 1;
                    break;
                }
            }
        }
        else if (*width != -1 && *height == -1)
        {
            // right edge
            for (int y = *border; y < image.height(); ++y)
            {
                if (is_bottom_right_corner(image, *width - 3, y - 2))
                {
                    *height = y + 1;
                    break;
                }
            }
        }
    }

    void try_top_right(const QImage& image, int* border, int* width, int* height)
    {
        if (image.pixel(image.width() - 1, 0) == qRgb(64, 64, 64)
            && image.pixel(image.width() - 2, 1) == qRgb(128, 128, 128)
            && image.pixel(image.width() - 3, 2) == qRgb(212, 208, 200))
        {
            if (image.pixel(image.width() - 4, 3) == qRgb(212, 208, 200))
                *border = 4;
            else
                *border = 3;
        }
        else
            return;

        // assume window is aligned at (0,0) so don't search for top left corner
        *width = image.width();

        // right edge
        for (int y = *border; y < image.height(); ++y)
        {
            if (is_bottom_right_corner(image, *width - 3, y - 2))
            {
                *height = y + 1;
                break;
            }
        }
    }
}

fake_window::fake_window(const site_settings::window_t& window, const site_settings& settings)
    : rect_(window.rect)
    , wid_(0)
    , title_font_(settings.get_font(window.font))
    , icon_(window.icon)
{
}

bool fake_window::update(WId wid)
{
    if (wid != -1)
        wid_ = wid;

    window_rect_ = QRect();
    client_rect_ = QRect();
    window_image_ = QImage();
    client_image_ = QImage();

    if (!IsWindow(reinterpret_cast<HWND>(wid_)))
        return false;

    const auto image = ::screenshot(wid_, rect_).toImage();

    // top left
    int border = -1;
    int width = -1;
    int height = -1;

    try_top_left(image, &border, &width, &height);

    if (border == -1 || width == -1 || height == -1)
        try_top_right(image, &border, &width, &height);

    if (border == -1)
        return false; // no window

    if (width == -1 || height == -1)
        throw std::runtime_error("Unable to determine window size");

    window_rect_ = QRect(rect_.left(), rect_.top(), width, height);

    title_rect_ = QRect(window_rect_.left() + border, window_rect_.top() + border,
        window_rect_.width() - 2 * border - WINDOW_BUTTONS_WIDTH, TITLE_HEIGHT);

    if (icon_)
        title_rect_.adjust(ICON_WIDTH + 2 * ICON_BORDER_X, 0, 0, 0);

    client_rect_ = window_rect_.adjusted(border, border + title_rect_.height() + TITLE_OFFSET, -border, -border);

    window_image_ = image.copy(window_rect_.translated(-window_rect_.topLeft()));
    client_image_ = image.copy(client_rect_.translated(-window_rect_.topLeft()));

    return is_valid();
}

bool fake_window::is_valid() const
{
    if (!IsWindow(reinterpret_cast<HWND>(wid_)))
        return false;

    if (!window_rect_.isValid() || !client_rect_.isValid() || window_image_.isNull() || client_image_.isNull())
        return false;

    return true;
}

bool fake_window::click_button(input_manager& input, const site_settings::button_t& button, bool double_click) const
{
    // check if button is there
    if (!window_utils::is_button(&client_image_, button))
        return false;

    const auto p = client_to_screen(QPoint(button.rect.x(), button.rect.y()));
    input.move_click(p.x(), p.y(), button.rect.width(), button.rect.height(), double_click);

    return true;
}

bool fake_window::click_any_button(input_manager& input, const site_settings::button_range& buttons, bool double_click) const
{
    for (const auto& i : buttons)
    {
        if (click_button(input, *i.second, double_click))
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

std::string fake_window::get_window_text() const
{
    const auto rect = title_rect_.translated(-window_rect_.topLeft()).adjusted(TITLE_TEXT_LEFT_OFFSET,
        TITLE_TEXT_TOP_OFFSET, TITLE_TEXT_BOTTOM_OFFSET, TITLE_TEXT_RIGHT_OFFSET);

    return window_utils::read_string(&window_image_, rect, qRgb(255, 255, 255), *title_font_);
}

const QImage& fake_window::get_window_image() const
{
    return window_image_;
}

const QImage& fake_window::get_client_image() const
{
    return client_image_;
}
