#pragma once

#pragma warning(push, 1)
#include <QPoint>
#include <QWidget>
#pragma warning(pop)

class window_manager
{
public:
    window_manager();
    QPoint client_to_screen(const QPoint& p) const;
    void update(const std::string& filter);
    const QImage& get_image() const;
    QRect get_tooltip() const;

private:
    WId wid_;
    QImage image_;
};
