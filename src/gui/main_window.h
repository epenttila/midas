#pragma once

#pragma warning(push, 3)
#include <QMainWindow>
#include <map>
#include <random>
#pragma warning(pop)

class QPlainTextEdit;
class holdem_abstraction;
class nlhe_state_base;
class site_base;
class strategy;
class QLabel;
class table_widget;
class QLineEdit;
class window_manager;
class holdem_strategy_widget;
class QComboBox;
class input_manager;
class lobby_base;
class QSpinBox;

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
    void play_changed();
    void play_timer_timeout();
    void step_triggered();
    void play_done_timeout();
    void lobby_timer_timeout();

private:
    struct strategy_info
    {
        strategy_info();
        ~strategy_info();
        std::unique_ptr<nlhe_state_base> root_state_;
        const nlhe_state_base* current_state_;
        std::unique_ptr<strategy> strategy_;
        std::string abstraction_;
    };

    struct snapshot_type
    {
        int round;
        double bet;
        int stack_size;
    };

    void find_window();
    void process_snapshot();
    void perform_action();
    void closeEvent(QCloseEvent* event);

    table_widget* visualizer_;
    std::map<int, std::unique_ptr<strategy_info>> strategy_infos_;
    QTimer* timer_;
    std::unique_ptr<site_base> site_;
    QLabel* strategy_label_;
    QLabel* capture_label_;
    QPlainTextEdit* log_;
    QComboBox* site_list_;
    QLineEdit* title_filter_;
    std::unique_ptr<window_manager> window_manager_;
    holdem_strategy_widget* strategy_;
    bool play_;
    QTimer* play_timer_;
    int next_action_;
    std::mt19937 engine_;
    double raise_fraction_;
    double capture_interval_;
    double action_delay_mean_;
    double action_delay_stddev_;
    std::unique_ptr<input_manager> input_manager_;
    snapshot_type snapshot_;
    bool acting_;
    QAction* step_action_;
    double action_min_delay_;
    std::map<std::string, std::unique_ptr<holdem_abstraction>> abstractions_;
    QTimer* play_done_timer_;
    double action_post_delay_;
    QTimer* lobby_timer_;
    std::unique_ptr<lobby_base> lobby_;
    QLineEdit* lobby_title_;
    double lobby_interval_;
    QSpinBox* table_count_;
    double action_max_delay_;
};
