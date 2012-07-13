#pragma warning(push, 3)
#include <QApplication>
#include "gui.h"
#pragma warning(pop)

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    Gui gui;
    gui.show();
    return application.exec();
}
