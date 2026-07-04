#pragma once

#include <QTcpSocket>

#include "comm/ILink.h"

namespace kerkenez {

class TcpLink : public ILink
{
    Q_OBJECT
public:
    explicit TcpLink(QString host, quint16 port, QObject *parent = nullptr);

    void open() override;
    void close() override;
    void send(const QByteArray &bytes) override;
    State state() const override { return m_state; }
    QString description() const override;

private:
    void setState(State state);

    QTcpSocket m_socket;
    QString m_host;
    quint16 m_port;
    State m_state = State::Disconnected;
};

} // namespace kerkenez
