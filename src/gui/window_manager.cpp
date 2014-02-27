#include "window_manager.h"

#pragma warning(push, 1)
#include <Windows.h>
#pragma warning(pop)

Q_GUI_EXPORT QPixmap qt_pixmapFromWinHBITMAP(HBITMAP, int hbitmapFormat = 0);

window_manager::window_manager()
{
}

void window_manager::update(const std::string& filter)
{
    wid_ = 0;
    image_ = QImage();

    if (filter.empty())
        return;

    const auto window = FindWindow(nullptr, filter.data());

    if (IsWindow(window) && IsWindowVisible(window))
        wid_ = reinterpret_cast<WId>(window);

    if (wid_ == 0)
        return;

    const auto hwnd = reinterpret_cast<HWND>(wid_);

    RECT rect;
    GetClientRect(hwnd, &rect);

    const HDC display_dc = GetDC(0);
    const HDC bitmap_dc = CreateCompatibleDC(display_dc);
    const HBITMAP bitmap = CreateCompatibleBitmap(display_dc, rect.right, rect.bottom);
    const HGDIOBJ null_bitmap = SelectObject(bitmap_dc, bitmap);

    const HDC window_dc = GetDC(hwnd);
    BitBlt(bitmap_dc, 0, 0, rect.right, rect.bottom, window_dc, 0, 0, SRCCOPY);
    ReleaseDC(hwnd, window_dc);

    SelectObject(bitmap_dc, null_bitmap);
    DeleteDC(bitmap_dc);

    const QPixmap pixmap = qt_pixmapFromWinHBITMAP(bitmap);

    DeleteObject(bitmap);
    ReleaseDC(0, display_dc);

    image_ = pixmap.toImage();
}

QPoint window_manager::client_to_screen(const QPoint& point) const
{
    POINT p = { point.x(), point.y() };
    ClientToScreen(reinterpret_cast<HWND>(wid_), &p);
    return QPoint(p.x, p.y);
}

const QImage& window_manager::get_image() const
{
    return image_;
}

namespace
{
    const auto INSIDE_COLOR = qRgb(255, 255, 225);
    const auto BORDER_COLOR = qRgb(0, 0, 0);

    QRect find_tooltip(const QImage& image, const int orig_x, const int orig_y)
    {
        // inside width
        int width = 0;

        for (int x = orig_x + 1; x < image.width() && image.pixel(x, orig_y + 1) == INSIDE_COLOR; ++x)
            ++width;

        if (width < 10)
            return QRect();

        // inside height
        int height = 0;

        for (int y = orig_y + 1; y < image.height() && image.pixel(orig_x + 1, y) == INSIDE_COLOR; ++y)
            ++height;

        if (height < 10)
            return QRect();

        // top and bottom border
        for (int x = orig_x; x < image.width() && x <= orig_x + width + 1; ++x)
        {
            if (image.pixel(x, orig_y) != BORDER_COLOR)
                return QRect();

            if (image.pixel(x, orig_y + height + 1) != BORDER_COLOR)
                return QRect();
        }

        // left and right border
        for (int y = orig_y; y < image.height() && y <= orig_y + height + 1; ++y)
        {
            if (image.pixel(orig_x, y) != BORDER_COLOR)
                return QRect();

            if (image.pixel(orig_x + width + 1, y) != BORDER_COLOR)
                return QRect();
        }

        // + 2 for borders
        return QRect(orig_x, orig_y, width + 2, height + 2);
    }
}

QRect window_manager::get_tooltip() const
{
    const auto& image = image_;

    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            const auto rect = find_tooltip(image, x, y);

            if (rect.isValid())
                return rect;
        }
    }

    return QRect();
}
