#pragma once

#pragma warning(push, 1)
#include <array>
#include <QWidget>
#pragma warning(pop)

class strategy;
class holdem_abstraction;

class holdem_strategy_widget : public QWidget
{
    Q_OBJECT

public:
    holdem_strategy_widget(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    void update(const holdem_abstraction& abs, const std::array<int, 2>& hole, const std::array<int, 5>& cards,
        const strategy& strategy, std::size_t state_id, int actions);

protected:
    virtual void paintEvent(QPaintEvent* event);

private:
    std::array<std::array<QColor, 52>, 52> actions_;
    std::array<int, 2> hole_;
};
