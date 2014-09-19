#pragma warning(push, 1)
#include <QApplication>
#include "main_window.h"
#include <QCommandLineParser>
#pragma warning(pop)

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QCommandLineParser parser;
    parser.addOption(QCommandLineOption("settings", "Path to settings file.", "file"));
    parser.process(application);
    main_window w(parser.value("settings").toStdString());
    w.show();
    return application.exec();
}
