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
#include "gamelib/nlhe_state.h"

class QPlainTextEdit;
class holdem_abstraction;
class table_manager;
class nlhe_strategy;
class QLabel;
class table_widget;
class QLineEdit;
class holdem_strategy_widget;
class QComboBox;
class input_manager;
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
    typedef int tid_t;
    typedef std::vector<std::unique_ptr<fake_window>> table_vector_t;

    main_window(const std::string& settings_path);
    ~main_window();

public slots:
    void capture_timer_timeout();
    void open_strategy();
    void show_strategy_changed();
    void autoplay_changed(bool checked);
    void modify_state_changed();
    void state_widget_board_changed(const QString& board);
    void schedule_changed(bool checked);
    void state_widget_state_changed();

private:
    struct table_data_t
    {
        table_data_t() : dealer(-1), big_blind(-1), stack_size(-1), state(nullptr),
            next_action(nlhe_state::INVALID_ACTION) {}

        int dealer;
        double big_blind;
        int stack_size;
        table_manager::snapshot_t snapshot;
        const nlhe_state* state;
        QDateTime next_action_time;
        nlhe_state::holdem_action next_action;
    };

    void process_snapshot(int slot, const fake_window& window);
    nlhe_state::holdem_action get_next_action(const nlhe_state& state, const nlhe_strategy& strategy,
        const table_manager::snapshot_t& snapshot);
    const nlhe_state* perform_action(nlhe_state::holdem_action action, const nlhe_state& state,
        const table_manager::snapshot_t& snapshot, double big_blind);
    void update_strategy_widget(const nlhe_state& state, const nlhe_strategy& strategy, const std::array<int, 2>& hole,
        const std::array<int, 5>& board);
    void ensure(bool expression, const std::string& s, int line) const;
    void update_statusbar();
    void handle_schedule();
    void load_settings(const std::string& filename);
    int get_effective_stack(const table_manager::snapshot_t& snapshot, double big_blind) const;
    bool is_new_game(const table_data_t& table_data, const table_manager::snapshot_t& snapshot) const;
    void save_snapshot() const;
    void update_capture();
    bool try_capture();
    void send_email(const std::string& subject, const std::string& message = ".");
    void check_idle(const bool schedule_active);
    void handle_error(const std::exception& e);

    table_widget* visualizer_;
    std::map<int, std::unique_ptr<nlhe_strategy>> strategies_;
    QTimer* capture_timer_;
    std::unique_ptr<table_manager> site_;
    QPlainTextEdit* log_;
    QLineEdit* title_filter_;
    holdem_strategy_widget* strategy_widget_;
    std::mt19937 engine_;
    std::unique_ptr<input_manager> input_manager_;
    QAction* autoplay_action_;
    state_widget* state_widget_;
    QAction* open_action_;
    QAction* save_images_;
    QAction* schedule_action_;
    bool schedule_active_;
    QLabel* schedule_label_;
    std::unordered_map<tid_t, table_data_t> table_data_;
    std::unordered_map<tid_t, table_data_t> old_table_data_;
    QLabel* site_label_;
    QLabel* strategy_label_;
    QDateTime next_activity_date_;
    smtp* smtp_;
    QTime mark_time_;
    std::unique_ptr<site_settings> settings_;
    std::unique_ptr<captcha_manager> captcha_manager_;
    std::unique_ptr<window_manager> window_manager_;
    double error_allowance_;
    QTime table_update_time_;
    table_vector_t table_windows_;
    QDateTime error_time_;
    double max_error_count_;
};
