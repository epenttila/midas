#pragma once

#pragma warning(push, 3)
#include <QWidget>
#pragma warning(pop)

class QTextEdit;
class holdem_abstraction;
class nl_holdem_state;
class site_stars;
class strategy;

class Gui : public QWidget
{
    Q_OBJECT

public:
    Gui();
    ~Gui();

public slots:
    void buttonClicked();
    void timerTimeout();

private:
    QTextEdit* text_;
    std::unique_ptr<holdem_abstraction> abstraction_;
    std::unique_ptr<nl_holdem_state> root_state_;
    const nl_holdem_state* current_state_;
    QTimer* timer_;
    std::unique_ptr<site_stars> site_;
    std::unique_ptr<strategy> strategy_;
};
