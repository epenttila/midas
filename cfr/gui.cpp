#pragma warning(push, 3)
#include <QtGui>
#include <Windows.h>
#pragma warning(pop)

#include "gui.h"
#include <array>
#include "card.h"

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

    double get_stack_size(const QImage& image, const QRect& rect)
    {
        const QRgb color = qRgb(192, 192, 192);
        std::string s;

        for (int x = rect.left(); x < rect.right(); )
        {
            char c = 0;

            switch (read_column(image, x, rect.top(), rect.top() + rect.height(), color))
            {
            case 0x7f8: c = '0'; x += 6; break;
            case 0x030: c = '1'; x += 4; break;
            case 0xc08: c = '2'; x += 6; break;
            case 0x408: c = '3'; x += 6; break;
            case 0x300: c = '4'; x += 6; break;
            case 0x470: c = '5'; x += 6; break;
            case 0x7f0: c = '6'; x += 6; break;
            case 0x004: c = '7'; x += 6; break;
            case 0x7b8: c = '8'; x += 6; break;
            case 0x478:
                {
                    switch (read_column(image, ++x, rect.top(), rect.top() + rect.height(), color))
                    {
                    case 0xcfc: c = '9'; x += 6; break;
                    }
                }
                break;
            case 0xc00: c = '.'; x += 2; break;
            }

            if (c)
                s += c;
            else
                ++x;
        }

        return std::atof(s.c_str());
    }

    int get_dealer(const QImage& image, const QPoint& point)
    {
        return (image.pixel(point) == qRgb(162, 97, 33)) ? 1 : 0;
    }

    std::string get_board_card(const QImage& image, const QRect& rect)
    {
        const std::array<QRgb, 4> colors = {{
            qRgb(41, 133, 10), // clubs
            qRgb(10, 10, 238), // diamonds
            qRgb(202, 18, 18), // hearts
            qRgb(2, 2, 2), // spades
        }};
        std::string s;

        for (int x = rect.left(); x < rect.right(); ++x)
        {
            for (int suit = 0; suit < 4; ++suit)
            {
                const QRgb& color = colors[suit];
                int rank = -1;

                switch (read_column(image, x, rect.top(), rect.top() + rect.height(), color))
                {
                case 0x1000: rank = 0; break; // 2
                case 0x0400: rank = 1; break; // 3
                case 0x0100: rank = 2; break; // 4
                case 0x0040: rank = 3; break; // 5
                case 0x00e0: rank = 4; break; // 6
                case 0x000f: rank = 5; break; // 7
                case 0x0300:
                    {
                        switch (read_column(image, ++x, rect.top(), rect.top() + rect.height(), color))
                        {
                        case 0x0f88: rank = 6; break; // 8
                        case 0x0f00: rank = 9; break; // J
                        }
                    }
                    break;
                case 0x0010: rank = 7; break; // 9
                case 0x1ffe: rank = 8; break; // 10
                case 0x0180: rank = 10; break; // Q
                case 0x1803: rank = 11; break; // K
                case 0x1800: rank = 12; break; // A
                }

                if (rank != -1)
                    return get_card_string(get_card(rank, suit));
            }
        }

        return "?";
    }
}

Gui::Gui()
{
    text_ = new QTextEdit(this);

    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(buttonClicked()));
    timer->start(1000);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(text_);

    setLayout(layout);
}

void Gui::buttonClicked()
{
    HWND hwnd = ::FindWindow("PokerStarsTableFrameClass", nullptr);

    if (!hwnd)
        return;

    auto test = QPixmap::grabWindow(hwnd).toImage();

    if (test.isNull())
        return;

    double stack = get_stack_size(test, QRect(36, 273, 91, 15));
    double stack2 = get_stack_size(test, QRect(671, 273, 91, 15));
    int dealer = get_dealer(test, QPoint(645, 269));
    std::string b = get_board_card(test, QRect(268, 157, 50, 13)) + " "
        + get_board_card(test, QRect(322, 157, 50, 13)) + " "
        + get_board_card(test, QRect(376, 157, 50, 13)) + " | "
        + get_board_card(test, QRect(430, 157, 50, 13)) + " "
        + get_board_card(test, QRect(484, 157, 50, 13));
    //double left_bet = QRect(128, 235, 100, 10);
    //double right_bet = QRect(534, 235, 136, 10);
    text_->setText(QString("stack1: %1\nstack2: %2\ndealer: %3\nb: %4").arg(stack).arg(stack2).arg(dealer).arg(b.c_str()));
    test.save("test.png");
}
