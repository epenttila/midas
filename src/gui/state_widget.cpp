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

state_widget::state_widget(QWidget* parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
    , state_(nullptr)
    , root_state_(nullptr)
{
    board_ = new QLineEdit();
    auto board_button = new QPushButton("Set");
    connect(board_button, &QPushButton::clicked, this, &state_widget::set_board_clicked);

    state_edit_ = new QLineEdit();
    state_edit_->setReadOnly(true);
    connect(state_edit_, &QLineEdit::textChanged, this, &state_widget::state_text_changed);

    auto state_button = new QPushButton("Reset");
    connect(state_button, &QPushButton::clicked, this, &state_widget::reset_clicked);

    auto call_button = new QPushButton("Call");
    connect(call_button, &QPushButton::clicked, this, &state_widget::call_clicked);

    raise_amount_ = new QDoubleSpinBox();
    raise_amount_->setRange(0.0, 1000.0);
    auto raise_button = new QPushButton("Raise");
    connect(raise_button, &QPushButton::clicked, this, &state_widget::raise_clicked);

    auto button_layout = new QHBoxLayout();
    button_layout->addWidget(call_button);
    button_layout->addWidget(raise_amount_);
    button_layout->addWidget(raise_button);

    auto state_layout = new QGridLayout();
    state_layout->addWidget(new QLabel("Board:"), 0, 0);
    state_layout->addWidget(board_, 0, 1);
    state_layout->addWidget(board_button, 0, 2);
    state_layout->addWidget(new QLabel("State:"), 1, 0);
    state_layout->addWidget(state_edit_, 1, 1);
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
    if (!state_)
        return;
    
    const auto next = state_->raise(raise_amount_->value());
    
    if (next && !next->is_terminal())
        set_state(next);
}

void state_widget::set_state(const nlhe_state* state)
{
    state_ = state;

    std::stringstream ss;

    if (state)
        ss << *state;

    state_edit_->setText(ss.str().c_str());
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
    set_state(root_state_);
}

void state_widget::state_text_changed()
{
    emit state_changed();
}

void state_widget::set_root_state(const nlhe_state* state)
{
    root_state_ = state;
    set_state(root_state_);
}
