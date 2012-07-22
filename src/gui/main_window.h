#pragma once

#pragma warning(push, 3)
#include <QMainWindow>
#include <map>
#pragma warning(pop)

class QTextEdit;
class holdem_abstraction;
class nl_holdem_state;
class site_stars;
class strategy;
class QLabel;
class table_widget;

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    main_window();
    ~main_window();

public slots:
    void timer_timeout();
    void open_strategy();
    void open_abstraction();

private:
    struct strategy_info
    {
        strategy_info();
        ~strategy_info();
        std::unique_ptr<nl_holdem_state> root_state_;
        const nl_holdem_state* current_state_;
        std::unique_ptr<strategy> strategy_;
    };

    void create_menus();

    table_widget* visualizer_;
    std::unique_ptr<holdem_abstraction> abstraction_;
    std::map<int, std::unique_ptr<strategy_info>> strategy_infos_;
    QTimer* timer_;
    std::unique_ptr<site_stars> site_;
    QLabel* abstraction_label_;
    QLabel* strategy_label_;
    QLabel* capture_label_;
    QLabel* decision_label_;
};
