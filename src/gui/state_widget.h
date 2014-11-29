#pragma once

#pragma warning(push, 1)
#include <QWidget>
#pragma warning(pop)

class QLineEdit;
class QDoubleSpinBox;
class nlhe_state;
class nlhe_strategy;

class state_widget : public QWidget
{
    Q_OBJECT

public:
    state_widget(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    void set_strategy(const nlhe_strategy* strategy);
    const nlhe_state* get_state() const;
    const nlhe_strategy* get_strategy() const;

public slots:
    void raise_clicked();
    void call_clicked();
    void reset_clicked();

signals:
    void board_changed(const QString& board);
    void state_changed();
    void stack_changed(const double val);

private:
    void set_state(const nlhe_state* state);

    QLineEdit* board_;
    QLineEdit* state_edit_;
    QDoubleSpinBox* raise_amount_;
    const nlhe_state* state_;
    const nlhe_strategy* strategy_;
    QDoubleSpinBox* stack_;
    QLineEdit* file_edit_;
};
