#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

#include "comm/LinkManager.h"

using namespace kerkenez;

class TestLinkManager : public QObject
{
    Q_OBJECT

private slots:
    void connectsReceivesAndSends()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));

        LinkManager manager;
        QSignalSpy stateSpy(&manager, &LinkManager::stateChanged);
        QSignalSpy bytesSpy(&manager, &LinkManager::bytesReceived);

        LinkConfig config;
        config.type = LinkConfig::Type::Tcp;
        config.host = QStringLiteral("127.0.0.1");
        config.port = server.serverPort();
        manager.connectLink(config);

        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 3000);
        QTcpSocket *peer = server.nextPendingConnection();
        QTRY_COMPARE(manager.state(), ILink::State::Connected);

        peer->write("hello");
        QTRY_COMPARE(bytesSpy.count(), 1);
        QCOMPARE(bytesSpy.first().at(0).toByteArray(), QByteArray("hello"));

        manager.send(QByteArrayLiteral("pong"));
        QTRY_VERIFY(peer->bytesAvailable() >= 4);
        QCOMPARE(peer->readAll(), QByteArray("pong"));

        manager.disconnectLink();
        QCOMPARE(manager.state(), ILink::State::Disconnected);
    }

    void reconnectsAfterRemoteDrop()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));

        LinkManager manager;
        manager.setReconnectIntervalMs(200);
        QSignalSpy reconnectSpy(&manager, &LinkManager::reconnectScheduled);

        LinkConfig config;
        config.type = LinkConfig::Type::Tcp;
        config.host = QStringLiteral("127.0.0.1");
        config.port = server.serverPort();
        manager.connectLink(config);

        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 3000);
        QTcpSocket *first = server.nextPendingConnection();
        QTRY_COMPARE(manager.state(), ILink::State::Connected);

        // Remote side drops the connection → manager must retry on its own.
        first->abort();
        QTRY_COMPARE(manager.state(), ILink::State::Disconnected);
        QVERIFY(reconnectSpy.count() >= 1);

        // QTRY keeps the event loop spinning — a blocking waitForNewConnection
        // would starve the manager's reconnect timer.
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 3000);
        QTRY_COMPARE(manager.state(), ILink::State::Connected);
    }

    void manualDisconnectStopsRetrying()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));

        LinkManager manager;
        manager.setReconnectIntervalMs(100);

        LinkConfig config;
        config.type = LinkConfig::Type::Tcp;
        config.host = QStringLiteral("127.0.0.1");
        config.port = server.serverPort();
        manager.connectLink(config);

        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 3000);
        server.nextPendingConnection();
        QTRY_COMPARE(manager.state(), ILink::State::Connected);

        manager.disconnectLink();
        QTest::qWait(400); // several retry intervals
        QVERIFY(!server.hasPendingConnections());
        QCOMPARE(manager.state(), ILink::State::Disconnected);
    }
};

QTEST_GUILESS_MAIN(TestLinkManager)
#include "tst_linkmanager.moc"
