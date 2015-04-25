#include "image_widget.h"

#pragma warning(push, 1)
#include <QPainter>
#pragma warning(pop)

const auto scale = 5;

image_widget::image_widget(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(800, 600);
}

void image_widget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawImage(0, 0, image_.scaled(image_.size() * scale));
}

void image_widget::set_image(const QImage& image)
{
    image_ = image;

    setFixedSize(image.size() * scale);

    update();
}

const QImage& image_widget::get_image() const
{
    return image_;
}
