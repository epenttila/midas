#pragma once

#pragma warning(push, 1)
#include <QWidget>
#include <QImage>
#pragma warning(pop)

class image_widget : public QWidget
{
    Q_OBJECT

public:
    image_widget(QWidget* parent);

    void paintEvent(QPaintEvent* event);
    void set_image(const QImage& image);
    const QImage& get_image() const;

public slots:

private:
    QImage image_;
};
