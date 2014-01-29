#pragma once

#pragma warning(push, 1)
#include <QWidget>
#pragma warning(pop)

class QLineEdit;
class QDoubleSpinBox;
class nlhe_state;

class state_widget : public QWidget
{
    Q_OBJECT

public:
    state_widget(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    void set_root_state(const nlhe_state* state);
    const nlhe_state* get_state() const;

public slots:
    void set_board_clicked();
    void raise_clicked();
    void call_clicked();
    void reset_clicked();
    void state_text_changed();

signals:
    void board_changed(const QString& board);
    void state_changed();

private:
    void set_state(const nlhe_state* state);

    QLineEdit* board_;
    QLineEdit* state_edit_;
    QDoubleSpinBox* raise_amount_;
    const nlhe_state* state_;
    const nlhe_state* root_state_;
};
