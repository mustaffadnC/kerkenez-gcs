#pragma once

#include <QTimer>

#include "comm/ILink.h"
#include "comm/LinkConfig.h"

namespace kerkenez {

// Owns the active link. Reconnects automatically while the user wants a
// connection; a manual disconnect stops the retry loop.
class LinkManager : public QObject
{
    Q_OBJECT
public:
    explicit LinkManager(QObject *parent = nullptr);

    void connectLink(const LinkConfig &config);
    void disconnectLink();

    ILink::State state() const;
    QString linkDescription() const;
    bool connectionWanted() const { return m_connectionWanted; }

    int reconnectIntervalMs() const { return m_reconnectIntervalMs; }
    void setReconnectIntervalMs(int ms) { m_reconnectIntervalMs = ms; }

public slots:
    void send(const QByteArray &bytes);

signals:
    void bytesReceived(const QByteArray &bytes);
    void stateChanged(kerkenez::ILink::State state);
    void errorOccurred(const QString &message);
    void reconnectScheduled(int delayMs);

private:
    ILink *createLink(const LinkConfig &config);
    void scheduleReconnect();

    ILink *m_link = nullptr;
    LinkConfig m_config;
    QTimer m_reconnectTimer;
    int m_reconnectIntervalMs = 3000;
    bool m_connectionWanted = false;
};

} // namespace kerkenez
