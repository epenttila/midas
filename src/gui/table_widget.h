#pragma once

#pragma warning(push, 3)
#include <array>
#include <memory>
#include <QFrame>
#pragma warning(pop)

class QPixmap;

class table_widget : public QFrame
{
    Q_OBJECT

public:
    table_widget(QWidget* parent);
    void set_board_cards(const std::array<int, 5>& board);
    void set_hole_cards(int seat, const std::array<int, 2>& cards);
    void set_dealer(int dealer);
    void set_pot(int round, const std::array<int, 2>& pot);
    void get_board_cards(std::array<int, 5>& board) const;
    void get_hole_cards(std::array<int, 2>& hole) const;
    void set_big_blind(double big_blind);
    void set_real_bets(double bet1, double bet2);
    void set_real_pot(double pot);
    void set_sit_out(bool sitout1, bool sitout2);
    void set_stacks(double stack1, double stack2);
    void set_buttons(int buttons);

protected:
    virtual void paintEvent(QPaintEvent* event);

private:
    std::array<int, 5> board_;
    std::array<std::array<int, 2>, 2> hole_;
    std::unique_ptr<QPixmap> dealer_image_;
    int dealer_;
    int round_;
    std::array<int, 2> bets_;
    std::array<int, 2> pot_;
    std::array<std::unique_ptr<QPixmap>, 52> card_images_;
    std::unique_ptr<QPixmap> empty_image_;
    double big_blind_;
    std::array<double, 2> real_bets_;
    double real_pot_;
    std::array<bool, 2> sit_out_;
    std::array<double, 2> stacks_;
    int buttons_;
};
