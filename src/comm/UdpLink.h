#pragma once

#include <QHostAddress>
#include <QUdpSocket>

#include "comm/ILink.h"

namespace kerkenez {

// Binds a local UDP port and learns the peer from the first incoming datagram
// (the usual GCS pattern: the autopilot is configured to send telemetry to us).
class UdpLink : public ILink
{
    Q_OBJECT
public:
    explicit UdpLink(quint16 localPort, QObject *parent = nullptr);

    void open() override;
    void close() override;
    void send(const QByteArray &bytes) override;
    State state() const override { return m_state; }
    QString description() const override;

private:
    void setState(State state);

    QUdpSocket m_socket;
    quint16 m_localPort;
    QHostAddress m_peerAddress;
    quint16 m_peerPort = 0;
    State m_state = State::Disconnected;
};

} // namespace kerkenez
