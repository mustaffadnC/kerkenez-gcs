#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QTimer>

#include "comm/LinkManager.h"
#include "core/GcsMessages.h"
#include "core/MavlinkCodec.h"
#include "core/Vehicle.h"
#include "ui/MainWindow.h"

using namespace kerkenez;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Kerkenez GCS"));
    QApplication::setApplicationVersion(QStringLiteral("0.2.0"));

    LinkManager linkManager;
    MavlinkCodec codec;
    Vehicle vehicle;

    // RX pipeline: link bytes → codec → vehicle state.
    QObject::connect(&linkManager, &LinkManager::bytesReceived, &codec, &MavlinkCodec::feed);
    QObject::connect(&codec, &MavlinkCodec::messageReceived, &vehicle, &Vehicle::handleMessage);

    // TX: 1 Hz GCS heartbeat; telemetry stream request on first contact.
    QTimer heartbeatTimer;
    QObject::connect(&heartbeatTimer, &QTimer::timeout, &linkManager, [&] {
        linkManager.send(makeGcsHeartbeat());
    });
    heartbeatTimer.start(1000);
    QObject::connect(&vehicle, &Vehicle::firstHeartbeat, &linkManager, [&](int systemId) {
        linkManager.send(makeStreamRequest(uint8_t(systemId)));
    });
    // A rebooted autopilot forgets stream rates — re-request on every reconnect.
    QObject::connect(&linkManager, &LinkManager::stateChanged, &vehicle, [&](ILink::State state) {
        if (state == ILink::State::Connected && vehicle.systemId() != 0)
            linkManager.send(makeStreamRequest(uint8_t(vehicle.systemId())));
    });

    MainWindow window(&linkManager, &codec, &vehicle);
    window.show();

    // e.g. --connect tcp:127.0.0.1:5760  or  --connect udp:14550
    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption connectOption(
        QStringLiteral("connect"), QStringLiteral("Auto-connect on startup."),
        QStringLiteral("endpoint"));
    parser.addOption(connectOption);
    // Dev/demo helper: save window frames (works even when covered by other
    // windows — QWidget::grab renders the widget tree, not the screen).
    const QCommandLineOption grabOption(
        QStringLiteral("grab"),
        QStringLiteral("Record window frames: <dir>[,count[,intervalMs]]"),
        QStringLiteral("spec"));
    parser.addOption(grabOption);
    const QCommandLineOption offlineMapOption(
        QStringLiteral("map-offline"),
        QStringLiteral("Draw the map from the tile cache only, never the network."));
    parser.addOption(offlineMapOption);
    parser.process(app);

    if (parser.isSet(offlineMapOption))
        window.setMapOffline(true);

    QTimer grabTimer;
    int grabIndex = 0;
    if (parser.isSet(grabOption)) {
        const QStringList spec = parser.value(grabOption).split(QLatin1Char(','));
        const QString dir = spec.value(0);
        const int count = spec.value(1, QStringLiteral("34")).toInt();
        const int intervalMs = spec.value(2, QStringLiteral("600")).toInt();
        QDir().mkpath(dir);
        QObject::connect(&grabTimer, &QTimer::timeout, &window, [&, dir, count] {
            window.grab().save(QStringLiteral("%1/frame%2.png")
                                   .arg(dir)
                                   .arg(grabIndex, 3, 10, QLatin1Char('0')));
            if (++grabIndex >= count)
                grabTimer.stop();
        });
        grabTimer.start(intervalMs);
    }
    if (parser.isSet(connectOption)) {
        const QStringList parts = parser.value(connectOption).split(QLatin1Char(':'));
        LinkConfig config;
        if (parts.size() >= 2 && parts[0] == QLatin1String("udp")) {
            config.type = LinkConfig::Type::Udp;
            config.port = parts[1].toUShort();
        } else if (parts.size() >= 3 && parts[0] == QLatin1String("tcp")) {
            config.type = LinkConfig::Type::Tcp;
            config.host = parts[1];
            config.port = parts[2].toUShort();
        }
        linkManager.connectLink(config);
    }

    return QApplication::exec();
}
