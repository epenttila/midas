#pragma once

#pragma warning(push, 1)
#include <QMainWindow>
#include <QFont>
#pragma warning(pop)

class QPlainTextEdit;
class image_widget;
class QLineEdit;
class QPushButton;
class QCheckBox;

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    main_window();

public slots:

private:
    void open_image();
    void generate_output();
    void create_font();

    image_widget* image_widget_;
    QPlainTextEdit* text_widget_;
    QPushButton* font_button_;
    QLineEdit* chars_widget_;
    QFont font_;
    QCheckBox* smooth_button_;
    QColor text_color_;
    QColor bg_color_;
};
