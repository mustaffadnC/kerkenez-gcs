#pragma once

#include <QDialog>

class QLabel;
class QLineEdit;
class QTableWidget;

namespace kerkenez {

class ParamController;

class ParamDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ParamDialog(ParamController *controller, QWidget *parent = nullptr);

private:
    void rebuild();
    void applyFilter();

    ParamController *m_controller;
    QLineEdit *m_filter = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_status = nullptr;
    bool m_updatingTable = false;
};

} // namespace kerkenez
