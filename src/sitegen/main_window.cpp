#include "main_window.h"

#pragma warning(push, 1)
#include <cstdint>
#include <QPlainTextEdit>
#include <QToolBar>
#include <QAction>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QPushButton>
#include <QFontDialog>
#include <QFontMetrics>
#include <QPainter>
#include <QLineEdit>
#include <QCheckBox>
#pragma warning(pop)

#include "image_widget.h"

main_window::main_window()
{
    auto widget = new QWidget(this);
    widget->setFocus();

    setCentralWidget(widget);

    image_widget_ = new image_widget(this);
    text_widget_ = new QPlainTextEdit(this);
    text_widget_->setReadOnly(true);
    text_widget_->setMaximumBlockCount(1000);

    auto toolbar = addToolBar("File");
    toolbar->setMovable(false);
    auto action = toolbar->addAction(QIcon(":/icons/folder_page_white.png"), "&Open...");
    connect(action, &QAction::triggered, this, &main_window::open_image);
    toolbar->addSeparator();
    font_button_ = new QPushButton("Font", this);
    toolbar->addWidget(font_button_);

    connect(font_button_, &QPushButton::clicked, [this]() {
        bool ok = false;
        const auto font = QFontDialog::getFont(&ok, font_, this);

        if (ok)
            font_ = font;
    });

    chars_widget_ = new QLineEdit(this);
    toolbar->addWidget(chars_widget_);

    connect(chars_widget_, &QLineEdit::textChanged, [this](const QString&) {
        create_font();
    });

    smooth_button_ = new QCheckBox("Anti-aliasing");
    connect(smooth_button_, &QCheckBox::toggled, [this](bool checked) {
        font_.setStyleStrategy(static_cast<QFont::StyleStrategy>(font_.styleStrategy()
            | (checked ? QFont::PreferAntialias : QFont::NoAntialias)));
    });

    toolbar->addWidget(smooth_button_);

    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->addWidget(image_widget_);
    layout->addWidget(text_widget_);
    widget->setLayout(layout);
}

void main_window::open_image()
{
    const auto filename = QFileDialog::getOpenFileName(this, "Open", QString(), "All files (*.*);;Image files (*.png)");

    image_widget_->set_image(QImage(filename));

    generate_output();
}

void main_window::generate_output()
{
    const auto& image = image_widget_->get_image();
    const auto& mono = image.convertToFormat(QImage::Format_Mono, Qt::ThresholdDither);

    QStringList list;
    int count = 0;
    int max_width = 0;

    QString text;

    for (int x = 0; x < image.width(); ++x)
    {
        if (image.pixel(x, 0) == qRgb(255, 0, 0))
        {
            if (list.size() > max_width)
                max_width = list.size();

            const auto ch = chars_widget_->text()[count++];
            text += QString("<entry mask=\"%1\" value=\"%2\"/>\n").arg(list.join(',')).arg(QString(ch).toHtmlEscaped());
            list.clear();
            continue;
        }

        std::uint32_t val = 0;

        for (int y = 0; y < image.height(); ++y)
        {
            if (mono.pixel(x, y) == qRgb(255, 255, 255))
                val |= 1 << y;
        }

        /*if (val == 0)
            continue;*/

        list.append(QString("0x%1").arg(val, 0, 16));
    }

    text = QString("<font id=\"\" max-width=\"%1\">\n").arg(max_width) + text + "</font>\n";

    text_widget_->clear();
    text_widget_->setPlainText(text);
}

void main_window::create_font()
{
    QFontMetrics fm(font_);

    QImage image(800, fm.height(), QImage::Format_ARGB32);
    image.fill(qRgb(0, 0, 0));

    QPainter painter(&image);
    painter.setFont(font_);

    int x = 0;

    QImage img(100, fm.height(), QImage::Format_Mono);
    QPainter p(&img);
    p.setFont(font_);
    p.setPen(qRgb(255, 255, 255));

    int trim_top = img.height();
    int trim_bottom = 0;

    for (const auto& ch : chars_widget_->text())
    {
        img.fill(qRgb(0, 0, 0));
        p.drawText(0, fm.ascent(), ch);

        int trim_left = img.width();
        int trim_right = 0;

        for (int char_y = 0; char_y < img.height(); ++char_y)
        {
            for (int char_x = 0; char_x < img.width(); ++char_x)
            {
                if (img.pixel(char_x, char_y) != qRgb(0, 0, 0))
                {
                    trim_left = std::min(trim_left, char_x);
                    trim_right = std::max(trim_right, char_x);
                    trim_top = std::min(trim_top, char_y);
                    trim_bottom = std::max(trim_bottom, char_y);
                }
            }
        }

        const auto r = QRect(trim_left, 0, trim_right - trim_left + 1, fm.height());

        painter.drawImage(QPoint(x, 0), img, r);
        x += r.width();

        painter.setPen(qRgb(255, 0, 0));
        painter.drawLine(x, 0, x, 100);
        ++x;
    }

    image_widget_->set_image(image.copy(0, trim_top, x, trim_bottom - trim_top + 1));

    generate_output();
}
