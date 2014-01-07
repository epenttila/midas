#pragma once

#pragma warning(push, 3)
#include <QMainWindow>
#include <map>
#include <random>
#include <fstream>
#include <array>
#include <memory>
#include <unordered_map>
#include <QDateTime>
#pragma warning(pop)

#include "table_manager.h"

class QPlainTextEdit;
class holdem_abstraction;
class nlhe_state_base;
class table_manager;
class nlhe_strategy;
class QLabel;
class table_widget;
class QLineEdit;
class holdem_strategy_widget;
class QComboBox;
class input_manager;
class lobby_manager;
class QSpinBox;
class state_widget;
class fake_window;
class smtp;

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    typedef std::array<std::vector<std::pair<double, double>>, 7> spans_t;

    main_window();
    ~main_window();

public slots:
    void capture_timer_timeout();
    void open_strategy();
    void show_strategy_changed();
    void autoplay_changed(bool checked);
    void modify_state_changed();
    void state_widget_board_changed(const QString& board);
    void schedule_changed(bool checked);
    void autolobby_changed(bool checked);
    void state_widget_state_changed();

private:
    struct table_data_t
    {
        table_data_t() : dealer(-1), initial_state(nullptr), state(nullptr) {}

        int dealer;

        table_manager::snapshot_t snapshot;

        const nlhe_state_base* initial_state;
        const nlhe_state_base* state;

        QDateTime next_action_time;
        QDateTime timestamp;
    };

    void process_snapshot(const fake_window& window);
    void perform_action(int tournament_id, const nlhe_strategy& strategy, const table_manager::snapshot_t& snapshot);
    bool nativeEvent(const QByteArray& eventType, void* message, long* result);
    void update_strategy_widget(int tournament_id, const nlhe_strategy& strategy, const std::array<int, 2>& hole,
        const std::array<int, 5>& board);
    void ensure(bool expression, const std::string& s, int line);
    void update_statusbar();
    void find_capture_window();
    void handle_lobby();
    void handle_schedule();
    void remove_old_table_data();

    table_widget* visualizer_;
    std::map<int, std::unique_ptr<nlhe_strategy>> strategies_;
    QTimer* capture_timer_;
    std::unique_ptr<table_manager> site_;
    QPlainTextEdit* log_;
    QLineEdit* title_filter_;
    holdem_strategy_widget* strategy_widget_;
    std::mt19937 engine_;
    std::array<double, 2> capture_interval_;
    std::unique_ptr<input_manager> input_manager_;
    std::array<double, 2> action_delay_;
    std::unique_ptr<lobby_manager> lobby_;
    QSpinBox* table_count_;
    QAction* autoplay_action_;
    state_widget* state_widget_;
    QAction* open_action_;
    QAction* save_images_;
    spans_t activity_spans_;
    QAction* schedule_action_;
    bool schedule_active_;
    QLabel* schedule_label_;
    QLabel* registered_label_;
    QAction* autolobby_action_;
    double activity_variance_;
    std::unordered_map<int, table_data_t> table_data_;
    QLabel* site_label_;
    QLabel* strategy_label_;
    double time_to_next_activity_;
    WId capture_window_;
    smtp* smtp_;
    QString smtp_from_;
    QString smtp_to_;
    QTime mark_time_;
    double mark_interval_;
};
