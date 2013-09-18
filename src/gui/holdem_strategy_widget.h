#pragma once

#pragma warning(push, 1)
#include <array>
#include <QWidget>
#pragma warning(pop)

class nlhe_strategy;
class holdem_abstraction_base;

class holdem_strategy_widget : public QWidget
{
    Q_OBJECT

public:
    holdem_strategy_widget(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    void update(const nlhe_strategy& strategy, const std::array<int, 2>& hole, const std::array<int, 5>& board,
        std::size_t state_id);

protected:
    virtual void paintEvent(QPaintEvent* event);
    bool event(QEvent* event);

private:
    std::array<std::array<QColor, 52>, 52> actions_;
    std::array<int, 2> hole_;
    std::array<std::array<QString, 52>, 52> action_names_;
};
