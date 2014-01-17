#pragma warning(push, 3)
#include <QApplication>
#include "main_window.h"
#pragma warning(pop)

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    main_window w;
    w.show();
    return application.exec();
}
