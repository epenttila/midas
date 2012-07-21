#pragma once

#pragma warning(push, 3)
#include <array>
#include <QWidget>
#pragma warning(pop)

class QPixmap;

class table_widget : public QWidget
{
    Q_OBJECT

public:
    table_widget(QWidget* parent);
    void set_board_cards(const std::array<int, 5>& board);
    void set_hole_cards(int seat, const std::array<int, 2>& cards);
    void set_dealer(int dealer);
    void set_pot(int round, const std::array<int, 2>& pot);

protected:
    virtual void paintEvent(QPaintEvent* event);

private:
    std::array<int, 5> board_;
    std::array<std::array<int, 2>, 2> hole_;
    std::unique_ptr<QPixmap> cards_image_;
    std::unique_ptr<QPixmap> dealer_image_;
    int dealer_;
    int round_;
    std::array<int, 2> bets_;
    std::array<int, 2> pot_;
};
