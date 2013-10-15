#pragma once

#pragma warning(push, 3)
#include <QMainWindow>
#include <map>
#include <random>
#include <fstream>
#include <array>
#include <memory>
#include <unordered_map>
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
    void process_snapshot(WId window);
    void perform_action(WId window, const nlhe_strategy& strategy);
    bool nativeEvent(const QByteArray& eventType, void* message, long* result);
    void update_strategy_widget(WId window, const nlhe_strategy& strategy, const std::array<int, 2>& hole,
        const std::array<int, 5>& board);
    void ensure(bool expression, const std::string& s, int line);

    table_widget* visualizer_;
    std::map<int, std::unique_ptr<nlhe_strategy>> strategies_;
    QTimer* capture_timer_;
    std::unique_ptr<table_manager> site_;
    QPlainTextEdit* log_;
    QLineEdit* title_filter_;
    std::unique_ptr<window_manager> window_manager_;
    holdem_strategy_widget* strategy_widget_;
    std::mt19937 engine_;
    double capture_interval_;
    std::unique_ptr<input_manager> input_manager_;
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
    double activity_variance_;
    std::unordered_map<WId, const nlhe_state_base*> states_;
};
