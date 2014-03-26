#pragma once

#pragma warning(push, 1)
#include <QMainWindow>
#include <map>
#include <random>
#include <array>
#include <memory>
#include <unordered_map>
#include <QDateTime>
#pragma warning(pop)

#include "table_manager.h"
#include "lobby_manager.h"

class QPlainTextEdit;
class holdem_abstraction;
class nlhe_state;
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
class site_settings;
class captcha_manager;
class window_manager;

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    typedef lobby_manager::tid_t tid_t;

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
        table_data_t() : dealer(-1), big_blind(-1), stack_size(-1), state(nullptr) {}

        int dealer;
        double big_blind;
        int stack_size;

        table_manager::snapshot_t snapshot;

        const nlhe_state* state;

        QDateTime next_action_time;
        QDateTime timestamp;
    };

    void process_snapshot(const fake_window& window);
    void perform_action(tid_t tournament_id, const nlhe_strategy& strategy, const table_manager::snapshot_t& snapshot,
        double big_blind);
    void update_strategy_widget(tid_t tournament_id, const nlhe_strategy& strategy, const std::array<int, 2>& hole,
        const std::array<int, 5>& board);
    void ensure(bool expression, const std::string& s, int line) const;
    void update_statusbar();
    void handle_lobby();
    void handle_schedule();
    void remove_old_table_data();
    void load_settings(const std::string& filename);
    int get_effective_stack(const table_manager::snapshot_t& snapshot, double big_blind) const;
    bool is_new_game(const table_data_t& table_data, const table_manager::snapshot_t& snapshot) const;
    void save_snapshot() const;
    void update_capture();

    table_widget* visualizer_;
    std::map<int, std::unique_ptr<nlhe_strategy>> strategies_;
    QTimer* capture_timer_;
    std::unique_ptr<table_manager> site_;
    QPlainTextEdit* log_;
    QLineEdit* title_filter_;
    holdem_strategy_widget* strategy_widget_;
    std::mt19937 engine_;
    std::unique_ptr<input_manager> input_manager_;
    std::unique_ptr<lobby_manager> lobby_;
    QSpinBox* table_count_;
    QAction* autoplay_action_;
    state_widget* state_widget_;
    QAction* open_action_;
    QAction* save_images_;
    QAction* schedule_action_;
    bool schedule_active_;
    QLabel* schedule_label_;
    QLabel* registered_label_;
    QAction* autolobby_action_;
    std::unordered_map<tid_t, table_data_t> table_data_;
    std::unordered_map<tid_t, table_data_t> old_table_data_;
    QLabel* site_label_;
    QLabel* strategy_label_;
    double time_to_next_activity_;
    smtp* smtp_;
    QTime mark_time_;
    std::unique_ptr<site_settings> settings_;
    std::unique_ptr<captcha_manager> captcha_manager_;
    std::unique_ptr<window_manager> window_manager_;
};
