#include "comm/LinkManager.h"

#include "comm/SerialLink.h"
#include "comm/TcpLink.h"
#include "comm/UdpLink.h"

namespace kerkenez {

LinkManager::LinkManager(QObject *parent)
    : QObject(parent)
{
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, [this] {
        if (m_connectionWanted && m_link && m_link->state() == ILink::State::Disconnected)
            m_link->open();
    });
}

void LinkManager::connectLink(const LinkConfig &config)
{
    disconnectLink();

    m_config = config;
    m_link = createLink(config);

    connect(m_link, &ILink::bytesReceived, this, &LinkManager::bytesReceived);
    connect(m_link, &ILink::errorOccurred, this, &LinkManager::errorOccurred);
    connect(m_link, &ILink::stateChanged, this, [this](ILink::State state) {
        emit stateChanged(state);
        if (state == ILink::State::Disconnected)
            scheduleReconnect();
    });

    m_connectionWanted = true;
    m_link->open();
}

void LinkManager::disconnectLink()
{
    m_connectionWanted = false;
    m_reconnectTimer.stop();
    if (m_link) {
        m_link->close();
        m_link->deleteLater();
        m_link = nullptr;
        emit stateChanged(ILink::State::Disconnected);
    }
}

ILink::State LinkManager::state() const
{
    return m_link ? m_link->state() : ILink::State::Disconnected;
}

QString LinkManager::linkDescription() const
{
    return m_link ? m_link->description() : QString();
}

void LinkManager::send(const QByteArray &bytes)
{
    if (m_link)
        m_link->send(bytes);
}

ILink *LinkManager::createLink(const LinkConfig &config)
{
    switch (config.type) {
    case LinkConfig::Type::Udp:
        return new UdpLink(config.port, this);
    case LinkConfig::Type::Serial:
        return new SerialLink(config.serialPortName, config.baudRate, this);
    case LinkConfig::Type::Tcp:
        break;
    }
    return new TcpLink(config.host, config.port, this);
}

void LinkManager::scheduleReconnect()
{
    if (!m_connectionWanted || m_reconnectTimer.isActive())
        return;
    m_reconnectTimer.start(m_reconnectIntervalMs);
    emit reconnectScheduled(m_reconnectIntervalMs);
}

} // namespace kerkenez
