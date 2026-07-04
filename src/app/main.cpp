#include <QApplication>
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

    return QApplication::exec();
}
