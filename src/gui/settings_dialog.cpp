#include "settings_dialog.h"

#pragma warning(push, 1)
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#pragma warning(pop)

settings_dialog::settings_dialog(double capture_interval, double action_min_delay, double action_delay_mean,
    double action_delay_stddev, double input_delay_mean, double input_delay_stddev, QWidget* parent)
    : QDialog(parent)
{
    auto layout = new QFormLayout(this);

    capture_interval_ = new QLineEdit(QString("%1").arg(capture_interval));
    layout->addRow("Capture interval:", capture_interval_);

    action_min_delay_ = new QLineEdit(QString("%1").arg(action_min_delay));
    layout->addRow("Action minimum delay:", action_min_delay_);
    action_delay_mean_ = new QLineEdit(QString("%1").arg(action_delay_mean));
    layout->addRow("Action delay mean:", action_delay_mean_);
    action_delay_stddev_ = new QLineEdit(QString("%1").arg(action_delay_stddev));
    layout->addRow("Action delay stddev:", action_delay_stddev_);

    input_delay_mean_ = new QLineEdit(QString("%1").arg(input_delay_mean));
    layout->addRow("Input delay mean:", input_delay_mean_);
    input_delay_stddev_ = new QLineEdit(QString("%1").arg(input_delay_stddev));
    layout->addRow("Input delay stddev:", input_delay_stddev_);

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), SLOT(reject()));
    layout->addWidget(buttons);

    setLayout(layout);
}

double settings_dialog::get_capture_interval() const
{
    return capture_interval_->text().toDouble();
}

double settings_dialog::get_action_delay_mean() const
{
    return action_delay_mean_->text().toDouble();
}

double settings_dialog::get_action_delay_stddev() const
{
    return action_delay_stddev_->text().toDouble();
}

double settings_dialog::get_input_delay_mean() const
{
    return input_delay_mean_->text().toDouble();
}

double settings_dialog::get_input_delay_stddev() const
{
    return input_delay_stddev_->text().toDouble();
}

double settings_dialog::get_action_min_delay() const
{
    return action_min_delay_->text().toDouble();
}
