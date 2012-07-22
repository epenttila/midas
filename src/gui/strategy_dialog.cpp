#include "strategy_dialog.h"

#pragma warning(push, 3)
#include <QDialog>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#pragma warning(pop)

strategy_dialog::strategy_dialog(const std::map<int, std::string>& filenames, QWidget* parent)
    : QDialog(parent)
{
    table_ = new QTableWidget(int(filenames.size()), 2, this);
    table_->setHorizontalHeaderLabels(QStringList() << "Stack" << "File");

    for (auto i = filenames.begin(); i != filenames.end(); ++i)
    {
        const int row = int(std::distance(filenames.begin(), i));
        table_->setItem(row, 0, new QTableWidgetItem(QString("%1").arg(i->first)));
        table_->setItem(row, 1, new QTableWidgetItem(i->second.c_str()));
    }

    table_->resizeColumnsToContents();

    auto add_button = new QPushButton("Add", this);
    connect(add_button, SIGNAL(clicked()), SLOT(add_button_clicked()));

    auto remove_button = new QPushButton("Remove", this);
    connect(remove_button, SIGNAL(clicked()), SLOT(remove_button_clicked()));

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), SLOT(reject()));

    auto addremove_layout = new QVBoxLayout;
    addremove_layout->addWidget(add_button, 0, Qt::AlignTop);
    addremove_layout->addWidget(remove_button, 1, Qt::AlignTop);

    auto table_layout = new QHBoxLayout;
    table_layout->addWidget(table_);
    table_layout->addLayout(addremove_layout);

    auto layout = new QVBoxLayout;
    layout->addLayout(table_layout);
    layout->addWidget(buttons);
    setLayout(layout);

    resize(800, 600);
}

void strategy_dialog::add_button_clicked()
{
    auto filenames = QFileDialog::getOpenFileNames(this, "Open strategy", QString(), "Strategy files (*.str)");

    for (auto i = filenames.begin(); i != filenames.end(); ++i)
    {
        int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, new QTableWidgetItem("0"));
        table_->setItem(row, 1, new QTableWidgetItem(*i));
    }

    table_->resizeColumnsToContents();
}

void strategy_dialog::remove_button_clicked()
{
    table_->removeRow(table_->currentRow());
}

std::map<int, std::string> strategy_dialog::get_filenames()
{
    std::map<int, std::string> m;

    for (int i = 0; i < table_->rowCount(); ++i)
        m[table_->item(i, 0)->text().toInt()] = table_->item(i, 1)->text().toStdString();

    return m;
}
