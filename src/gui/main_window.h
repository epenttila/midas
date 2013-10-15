#pragma once

#pragma warning(push, 3)
#include <QMainWindow>
#include <map>
#include <random>
#include <fstream>
#include <array>
#include <memory>
#pragma warning(pop)

class QPlainTextEdit;
class holdem_abstraction;
class nlhe_state_base;
class table_manager;
class nlhe_strategy;
class QLabel;
class table_widget;
class QLineEdit;
class window_manager;
class holdem_strategy_widget;
class QComboBox;
class input_manager;
class lobby_manager;
class QSpinBox;
class state_widget;

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    main_window();
    ~main_window();

public slots:
    void capture_timer_timeout();
    void open_strategy();
    void show_strategy_changed();
    void autoplay_changed(bool checked);
    void action_start_timeout();
    void action_finish_timeout();
    void autolobby_timer_timeout();
    void modify_state_changed();
    void state_widget_board_changed(const QString& board);
    void schedule_changed(bool checked);
    void schedule_timer_timeout();
    void registration_timer_timeout();
    void autolobby_changed(bool checked);
    void table_title_changed(const QString& str);
    void state_widget_state_changed();

private:
    struct strategy_info
    {
        strategy_info();
        ~strategy_info();
        const nlhe_state_base* current_state_;
        std::unique_ptr<nlhe_strategy> strategy_;
    };

    void find_window();
    void process_snapshot();
    void perform_action();
    bool nativeEvent(const QByteArray& eventType, void* message, long* result);
    void update_strategy_widget(const strategy_info& si);
    void ensure(bool expression, const std::string& s, int line);

    table_widget* visualizer_;
    std::map<int, std::unique_ptr<strategy_info>> strategy_infos_;
    QTimer* capture_timer_;
    std::unique_ptr<table_manager> site_;
    QLabel* capture_label_;
    QPlainTextEdit* log_;
    QLineEdit* title_filter_;
    std::unique_ptr<window_manager> window_manager_;
    holdem_strategy_widget* strategy_;
    int next_action_;
    std::mt19937 engine_;
    double raise_fraction_;
    double capture_interval_;
    std::unique_ptr<input_manager> input_manager_;
    bool acting_;
    std::array<double, 2> action_delay_;
    double action_post_delay_;
    QTimer* autolobby_timer_;
    std::unique_ptr<lobby_manager> lobby_;
    QLineEdit* lobby_title_;
    double lobby_interval_;
    QSpinBox* table_count_;
    QAction* autoplay_action_;
    state_widget* state_widget_;
    QAction* open_action_;
    QAction* save_images_;
    std::vector<std::pair<double, double>> activity_spans_;
    QAction* schedule_action_;
    bool schedule_active_;
    QTimer* schedule_timer_;
    QLabel* schedule_label_;
    QTimer* registration_timer_;
    QLabel* registered_label_;
    QAction* autolobby_action_;
    int stack_size_;
    double activity_variance_;
};
