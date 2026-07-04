#pragma once

#include <QDialog>

#include "comm/LinkConfig.h"

class QComboBox;
class QLineEdit;
class QSpinBox;
class QStackedWidget;

namespace kerkenez {

class ConnectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConnectDialog(QWidget *parent = nullptr);

    LinkConfig config() const;

private:
    void refreshSerialPorts();

    QComboBox *m_typeCombo = nullptr;
    QStackedWidget *m_pages = nullptr;

    QLineEdit *m_tcpHost = nullptr;
    QSpinBox *m_tcpPort = nullptr;
    QSpinBox *m_udpPort = nullptr;
    QComboBox *m_serialPort = nullptr;
    QComboBox *m_serialBaud = nullptr;
};

} // namespace kerkenez
