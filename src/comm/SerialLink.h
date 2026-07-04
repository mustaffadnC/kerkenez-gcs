#pragma once

#include <QSerialPort>

#include "comm/ILink.h"

namespace kerkenez {

class SerialLink : public ILink
{
    Q_OBJECT
public:
    SerialLink(const QString &portName, qint32 baudRate, QObject *parent = nullptr);

    void open() override;
    void close() override;
    void send(const QByteArray &bytes) override;
    State state() const override { return m_state; }
    QString description() const override;

private:
    void setState(State state);

    QSerialPort m_port;
    State m_state = State::Disconnected;
};

} // namespace kerkenez
