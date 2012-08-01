#pragma once

#pragma warning(push, 3)
#include <QMainWindow>
#include <map>
#pragma warning(pop)

class QTextEdit;
class holdem_abstraction;
class nlhe_state_base;
class site_stars;
class strategy;
class QLabel;
class table_widget;
class QLineEdit;
class window_manager;
class holdem_strategy_widget;

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    main_window();
    ~main_window();

public slots:
    void timer_timeout();
    void open_strategy();
    void capture_changed();
    void show_strategy_changed();

private:
    struct strategy_info
    {
        strategy_info();
        ~strategy_info();
        std::unique_ptr<nlhe_state_base> root_state_;
        const nlhe_state_base* current_state_;
        std::unique_ptr<strategy> strategy_;
        std::unique_ptr<holdem_abstraction> abstraction_;
    };

    table_widget* visualizer_;
    std::map<int, std::unique_ptr<strategy_info>> strategy_infos_;
    QTimer* timer_;
    std::unique_ptr<site_stars> site_;
    QLabel* strategy_label_;
    QLabel* capture_label_;
    QLabel* decision_label_;
    QLineEdit* class_filter_;
    QLineEdit* title_filter_;
    std::unique_ptr<window_manager> window_manager_;
    holdem_strategy_widget* strategy_;
};
