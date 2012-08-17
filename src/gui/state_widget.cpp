#include "state_widget.h"

#pragma warning(push, 1)
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QLabel>
#pragma warning(pop)

state_widget::state_widget(QWidget* parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
{
    board_ = new QLineEdit();
    auto board_button = new QPushButton("Set");
    connect(board_button, SIGNAL(clicked()), SLOT(set_board_clicked()));

    state_ = new QLineEdit();
    state_->setReadOnly(true);
    auto state_button = new QPushButton("Reset");
    connect(state_button, SIGNAL(clicked()), SIGNAL(state_reset()));

    auto call_button = new QPushButton("Call");
    connect(call_button, SIGNAL(clicked()), SIGNAL(called()));

    raise_amount_ = new QDoubleSpinBox();
    raise_amount_->setRange(0.0, 1000.0);
    auto raise_button = new QPushButton("Raise");
    connect(raise_button, SIGNAL(clicked()), SLOT(raise_clicked()));

    auto button_layout = new QHBoxLayout();
    button_layout->addWidget(call_button);
    button_layout->addWidget(raise_amount_);
    button_layout->addWidget(raise_button);

    auto state_layout = new QGridLayout();
    state_layout->addWidget(new QLabel("Board:"), 0, 0);
    state_layout->addWidget(board_, 0, 1);
    state_layout->addWidget(board_button, 0, 2);
    state_layout->addWidget(new QLabel("State:"), 1, 0);
    state_layout->addWidget(state_, 1, 1);
    state_layout->addWidget(state_button, 1, 2);

    auto layout = new QVBoxLayout();
    layout->addLayout(state_layout);
    layout->addLayout(button_layout);

    setLayout(layout);
}

void state_widget::set_board_clicked()
{
    emit board_changed(board_->text());
}

void state_widget::raise_clicked()
{
    emit raised(raise_amount_->value());
}

void state_widget::set_state(const QString& state)
{
    state_->setText(state);
}
