#pragma once

#pragma warning(push, 1)
#include <array>
#include <QWidget>
#pragma warning(pop)

class nlhe_strategy;
class holdem_abstraction_base;
class nlhe_state_base;

class holdem_strategy_widget : public QWidget
{
    Q_OBJECT

public:
    struct cell
    {
        QColor color;
        QString names;
        int bucket;
    };

    holdem_strategy_widget(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    void update(const nlhe_strategy& strategy, const nlhe_state_base& state);
    void set_hole(const std::array<int, 2>& hole);
    void set_board(const std::array<int, 5>& board);

protected:
    virtual void paintEvent(QPaintEvent* event);
    bool event(QEvent* event);
    void mousePressEvent(QMouseEvent* event);

private:
    std::array<std::array<cell, 52>, 52> cells_;
    std::array<int, 2> hole_;
    int hilight_bucket_;
    std::array<int, 5> board_;
};
