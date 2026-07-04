#include "ui/MainWindow.h"

#include <QLabel>
#include <QStatusBar>

namespace kerkenez {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Kerkenez GCS"));
    resize(1024, 640);

    auto *placeholder = new QLabel(
        QStringLiteral("Kerkenez GCS — Phase 0 skeleton\n\n"
                       "Telemetry panel, map and mission editor arrive in later phases."),
        this);
    placeholder->setAlignment(Qt::AlignCenter);
    setCentralWidget(placeholder);

    statusBar()->showMessage(QStringLiteral("Disconnected"));
}

} // namespace kerkenez
