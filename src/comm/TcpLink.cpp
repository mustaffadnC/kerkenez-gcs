#include "comm/TcpLink.h"

namespace kerkenez {

TcpLink::TcpLink(QString host, quint16 port, QObject *parent)
    : ILink(parent)
    , m_host(std::move(host))
    , m_port(port)
{
    connect(&m_socket, &QTcpSocket::readyRead, this, [this] {
        emit bytesReceived(m_socket.readAll());
    });
    connect(&m_socket, &QTcpSocket::connected, this, [this] {
        setState(State::Connected);
    });
    connect(&m_socket, &QTcpSocket::disconnected, this, [this] {
        setState(State::Disconnected);
    });
    connect(&m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(m_socket.errorString());
        setState(State::Disconnected);
    });
}

void TcpLink::open()
{
    if (m_state != State::Disconnected)
        return;
    setState(State::Connecting);
    m_socket.connectToHost(m_host, m_port);
}

void TcpLink::close()
{
    m_socket.close();
    setState(State::Disconnected);
}

void TcpLink::send(const QByteArray &bytes)
{
    if (m_state == State::Connected)
        m_socket.write(bytes);
}

QString TcpLink::description() const
{
    return QStringLiteral("tcp://%1:%2").arg(m_host).arg(m_port);
}

void TcpLink::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

} // namespace kerkenez
