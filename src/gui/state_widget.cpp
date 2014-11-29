#include "state_widget.h"

#pragma warning(push, 1)
#include <sstream>
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QLabel>
#pragma warning(pop)

#include "gamelib/nlhe_state.h"
#include "cfrlib/nlhe_strategy.h"

state_widget::state_widget(QWidget* parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
    , state_(nullptr)
    , strategy_(nullptr)
{
    board_ = new QLineEdit();
    connect(board_, &QLineEdit::textChanged, [this](const QString& val) {
        emit board_changed(val);
    });

    state_edit_ = new QLineEdit();
    state_edit_->setReadOnly(true);

    auto state_button = new QPushButton("Reset");
    connect(state_button, &QPushButton::clicked, this, &state_widget::reset_clicked);

    auto call_button = new QPushButton("Call");
    connect(call_button, &QPushButton::clicked, this, &state_widget::call_clicked);

    raise_amount_ = new QDoubleSpinBox();
    raise_amount_->setRange(0.0, 1000.0);
    auto raise_button = new QPushButton("Raise");
    connect(raise_button, &QPushButton::clicked, this, &state_widget::raise_clicked);

    stack_ = new QDoubleSpinBox();
    stack_->setRange(0.0, 1000.0);
    connect(stack_, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
        [this](const double val)
    {
        emit stack_changed(val);
    });

    file_edit_ = new QLineEdit();
    file_edit_->setReadOnly(true);

    auto button_layout = new QHBoxLayout();
    button_layout->addWidget(call_button, Qt::AlignBottom);
    button_layout->addWidget(raise_amount_, Qt::AlignBottom);
    button_layout->addWidget(raise_button, Qt::AlignBottom);

    auto state_layout = new QGridLayout();
    state_layout->addWidget(new QLabel("Stack:"), 0, 0);
    state_layout->addWidget(stack_, 0, 1, 1, 2);
    state_layout->addWidget(new QLabel("File:"), 1, 0);
    state_layout->addWidget(file_edit_, 1, 1, 1, 2);
    state_layout->addWidget(new QLabel("Board:"), 2, 0);
    state_layout->addWidget(board_, 2, 1, 1, 2);
    state_layout->addWidget(new QLabel("State:"), 3, 0);
    state_layout->addWidget(state_edit_, 3, 1);
    state_layout->addWidget(state_button, 3, 2);

    auto layout = new QVBoxLayout();
    layout->addLayout(state_layout);
    layout->addLayout(button_layout);

    setLayout(layout);
}

void state_widget::raise_clicked()
{
    if (!state_)
        return;
    
    const auto next = state_->raise(raise_amount_->value());
    
    if (next && !next->is_terminal())
        set_state(next);
}

void state_widget::set_state(const nlhe_state* state)
{
    if (state == state_)
        return;

    state_ = state;

    std::stringstream ss;

    if (state)
        ss << *state;

    state_edit_->setText(ss.str().c_str());

    emit state_changed();
}

const nlhe_state* state_widget::get_state() const
{
    return state_;
}

void state_widget::call_clicked()
{
    if (state_ && state_->call() && !state_->call()->is_terminal())
        set_state(state_->call());
}

void state_widget::reset_clicked()
{
    set_state(strategy_ ? &strategy_->get_root_state() : nullptr);
}

void state_widget::set_strategy(const nlhe_strategy* strategy)
{
    if (strategy == strategy_)
        return;

    strategy_ = strategy;
    file_edit_->setText(strategy ? QString::fromStdString(strategy->get_strategy().get_filename()) : "N/A");
    reset_clicked();
}

const nlhe_strategy* state_widget::get_strategy() const
{
    return strategy_;
}
