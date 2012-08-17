#pragma once

#pragma warning(push, 1)
#include <QWidget>
#pragma warning(pop)

class QLineEdit;
class QDoubleSpinBox;

class state_widget : public QWidget
{
    Q_OBJECT

public:
    state_widget(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    void set_state(const QString& state);

public slots:
    void set_board_clicked();
    void raise_clicked();

signals:
    void board_changed(const QString& board);
    void state_reset();
    void called();
    void raised(double fraction);

private:
    QLineEdit* board_;
    QLineEdit* state_;
    QDoubleSpinBox* raise_amount_;
};
