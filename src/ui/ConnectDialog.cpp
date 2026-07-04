#include "ui/ConnectDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace kerkenez {

ConnectDialog::ConnectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Connect"));

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItems({tr("TCP (SITL)"), tr("UDP"), tr("Serial")});

    m_pages = new QStackedWidget(this);

    // TCP page
    auto *tcpPage = new QWidget(this);
    auto *tcpForm = new QFormLayout(tcpPage);
    m_tcpHost = new QLineEdit(QStringLiteral("127.0.0.1"), tcpPage);
    m_tcpPort = new QSpinBox(tcpPage);
    m_tcpPort->setRange(1, 65535);
    m_tcpPort->setValue(5760);
    tcpForm->addRow(tr("Host"), m_tcpHost);
    tcpForm->addRow(tr("Port"), m_tcpPort);
    m_pages->addWidget(tcpPage);

    // UDP page
    auto *udpPage = new QWidget(this);
    auto *udpForm = new QFormLayout(udpPage);
    m_udpPort = new QSpinBox(udpPage);
    m_udpPort->setRange(1, 65535);
    m_udpPort->setValue(14550);
    udpForm->addRow(tr("Local port"), m_udpPort);
    m_pages->addWidget(udpPage);

    // Serial page
    auto *serialPage = new QWidget(this);
    auto *serialForm = new QFormLayout(serialPage);
    m_serialPort = new QComboBox(serialPage);
    m_serialBaud = new QComboBox(serialPage);
    for (const qint32 baud : {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600})
        m_serialBaud->addItem(QString::number(baud), baud);
    m_serialBaud->setCurrentText(QStringLiteral("115200"));
    auto *refreshButton = new QPushButton(tr("Refresh"), serialPage);
    connect(refreshButton, &QPushButton::clicked, this, &ConnectDialog::refreshSerialPorts);
    serialForm->addRow(tr("Port"), m_serialPort);
    serialForm->addRow(tr("Baud"), m_serialBaud);
    serialForm->addRow(QString(), refreshButton);
    m_pages->addWidget(serialPage);
    refreshSerialPorts();

    connect(m_typeCombo, &QComboBox::currentIndexChanged, m_pages, &QStackedWidget::setCurrentIndex);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_typeCombo);
    layout->addWidget(m_pages);
    layout->addWidget(buttons);
}

LinkConfig ConnectDialog::config() const
{
    LinkConfig cfg;
    switch (m_typeCombo->currentIndex()) {
    case 1:
        cfg.type = LinkConfig::Type::Udp;
        cfg.port = quint16(m_udpPort->value());
        break;
    case 2:
        cfg.type = LinkConfig::Type::Serial;
        cfg.serialPortName = m_serialPort->currentText();
        cfg.baudRate = m_serialBaud->currentData().toInt();
        break;
    default:
        cfg.type = LinkConfig::Type::Tcp;
        cfg.host = m_tcpHost->text().trimmed();
        cfg.port = quint16(m_tcpPort->value());
        break;
    }
    return cfg;
}

void ConnectDialog::refreshSerialPorts()
{
    m_serialPort->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports)
        m_serialPort->addItem(info.portName());
}

} // namespace kerkenez
