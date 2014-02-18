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
