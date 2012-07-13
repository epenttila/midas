#pragma once

#pragma warning(push, 3)
#include <QWidget>
#pragma warning(pop)

class QTextEdit;

class Gui : public QWidget
{
    Q_OBJECT

public:
    Gui();

public slots:
    void buttonClicked();

private:
    QTextEdit* text_;
};
