#include <QApplication>

#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Kerkenez GCS"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    kerkenez::MainWindow window;
    window.show();

    return QApplication::exec();
}
