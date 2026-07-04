#include "comm/UdpLink.h"

namespace kerkenez {

UdpLink::UdpLink(quint16 localPort, QObject *parent)
    : ILink(parent)
    , m_localPort(localPort)
{
    connect(&m_socket, &QUdpSocket::readyRead, this, [this] {
        QByteArray assembled;
        while (m_socket.hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(int(m_socket.pendingDatagramSize()));
            QHostAddress sender;
            quint16 senderPort = 0;
            m_socket.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
            if (m_peerPort == 0) {
                m_peerAddress = sender;
                m_peerPort = senderPort;
            }
            assembled += datagram;
        }
        if (!assembled.isEmpty())
            emit bytesReceived(assembled);
    });
    connect(&m_socket, &QUdpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(m_socket.errorString());
        setState(State::Disconnected);
    });
}

void UdpLink::open()
{
    if (m_state != State::Disconnected)
        return;
    if (m_socket.bind(QHostAddress::AnyIPv4, m_localPort)) {
        setState(State::Connected);
    } else {
        emit errorOccurred(m_socket.errorString());
        setState(State::Disconnected);
    }
}

void UdpLink::close()
{
    m_socket.close();
    m_peerPort = 0;
    setState(State::Disconnected);
}

void UdpLink::send(const QByteArray &bytes)
{
    if (m_state == State::Connected && m_peerPort != 0)
        m_socket.writeDatagram(bytes, m_peerAddress, m_peerPort);
}

QString UdpLink::description() const
{
    return QStringLiteral("udp://0.0.0.0:%1").arg(m_localPort);
}

void UdpLink::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

} // namespace kerkenez
