#include "comm/SerialLink.h"

namespace kerkenez {

SerialLink::SerialLink(const QString &portName, qint32 baudRate, QObject *parent)
    : ILink(parent)
{
    m_port.setPortName(portName);
    m_port.setBaudRate(baudRate);

    connect(&m_port, &QSerialPort::readyRead, this, [this] {
        emit bytesReceived(m_port.readAll());
    });
    connect(&m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error == QSerialPort::NoError)
            return;
        emit errorOccurred(m_port.errorString());
        if (m_port.isOpen())
            m_port.close();
        setState(State::Disconnected);
    });
}

void SerialLink::open()
{
    if (m_state != State::Disconnected)
        return;
    if (m_port.open(QIODevice::ReadWrite)) {
        setState(State::Connected);
    } else {
        emit errorOccurred(m_port.errorString());
        setState(State::Disconnected);
    }
}

void SerialLink::close()
{
    if (m_port.isOpen())
        m_port.close();
    setState(State::Disconnected);
}

void SerialLink::send(const QByteArray &bytes)
{
    if (m_state == State::Connected)
        m_port.write(bytes);
}

QString SerialLink::description() const
{
    return QStringLiteral("serial://%1@%2").arg(m_port.portName()).arg(m_port.baudRate());
}

void SerialLink::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

} // namespace kerkenez
