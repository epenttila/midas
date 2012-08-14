#pragma once

#pragma warning(push, 1)
#include <QDialog>
#pragma warning(pop)

class QLineEdit;

class settings_dialog : public QDialog
{
    Q_OBJECT

public:
    settings_dialog(double capture_interval, double action_min_delay, double action_delay_mean, double action_delay_stddev,
        double input_delay_mean, double input_delay_stddev, QWidget* parent);
    double get_capture_interval() const;
    double get_action_delay_mean() const;
    double get_action_delay_stddev() const;
    double get_input_delay_mean() const;
    double get_input_delay_stddev() const;
    double get_action_min_delay() const;

private:
    QLineEdit* capture_interval_;
    QLineEdit* action_delay_mean_;
    QLineEdit* action_delay_stddev_;
    QLineEdit* input_delay_mean_;
    QLineEdit* input_delay_stddev_;
    QLineEdit* action_min_delay_;
};
