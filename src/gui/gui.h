#pragma once

#pragma warning(push, 3)
#include <QMainWindow>
#pragma warning(pop)

class QTextEdit;
class holdem_abstraction;
class nl_holdem_state;
class site_stars;
class strategy;
class QLabel;
class table_widget;

class Gui : public QMainWindow
{
    Q_OBJECT

public:
    Gui();
    ~Gui();

public slots:
    void timerTimeout();
    void open_strategy();
    void open_abstraction();

private:
    void create_menus();

    table_widget* visualizer_;
    std::unique_ptr<holdem_abstraction> abstraction_;
    std::unique_ptr<nl_holdem_state> root_state_;
    const nl_holdem_state* current_state_;
    QTimer* timer_;
    std::unique_ptr<site_stars> site_;
    std::unique_ptr<strategy> strategy_;
    QLabel* abstraction_label_;
    QLabel* strategy_label_;
    QLabel* capture_label_;
    QLabel* decision_label_;
};
