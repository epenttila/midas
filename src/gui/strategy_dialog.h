#pragma once

#pragma warning(push, 3)
#include <QDialog>
#include <map>
#pragma warning(pop)

class QTableWidget;

class strategy_dialog : public QDialog
{
    Q_OBJECT

public:
    strategy_dialog(const std::map<int, std::string>& filenames, QWidget* parent);
    std::map<int, std::string> get_filenames();

public slots:
    void add_button_clicked();
    void remove_button_clicked();

private:
    QTableWidget* table_;
};
