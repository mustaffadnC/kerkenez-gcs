#pragma once

#include <QMainWindow>

namespace kerkenez {

// Phase 0 placeholder shell — panels arrive in later phases.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
};

} // namespace kerkenez
