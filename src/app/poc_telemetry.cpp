// Console proof of the full comm pipeline: LinkManager (auto-reconnect) →
// MavlinkCodec → Vehicle. Prints live telemetry; survives SITL restarts.
//
// Usage: poc_telemetry [host] [port]   (defaults: 127.0.0.1 5760)

#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

#include "comm/LinkManager.h"
#include "core/GcsMessages.h"
#include "core/MavlinkCodec.h"
#include "core/Vehicle.h"

using namespace kerkenez;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    LinkConfig config;
    config.type = LinkConfig::Type::Tcp;
    config.host = argc > 1 ? argv[1] : QStringLiteral("127.0.0.1");
    config.port = argc > 2 ? static_cast<quint16>(QString(argv[2]).toUShort()) : 5760;

    LinkManager linkManager;
    MavlinkCodec codec;
    Vehicle vehicle;

    QObject::connect(&linkManager, &LinkManager::bytesReceived, &codec, &MavlinkCodec::feed);
    QObject::connect(&codec, &MavlinkCodec::messageReceived, &vehicle, &Vehicle::handleMessage);

    QTimer heartbeatTimer;
    QObject::connect(&heartbeatTimer, &QTimer::timeout, &linkManager, [&] {
        linkManager.send(makeGcsHeartbeat());
    });
    heartbeatTimer.start(1000);

    QObject::connect(&vehicle, &Vehicle::firstHeartbeat, &linkManager, [&](int systemId) {
        out << "[poc ] vehicle sysid=" << systemId << ", requesting streams" << Qt::endl;
        linkManager.send(makeStreamRequest(uint8_t(systemId)));
    });

    QObject::connect(&linkManager, &LinkManager::stateChanged, [&](ILink::State state) {
        out << "[link] "
            << (state == ILink::State::Connected     ? "connected"
                : state == ILink::State::Connecting  ? "connecting"
                                                     : "disconnected")
            << Qt::endl;
        // A rebooted autopilot forgets stream rates — re-request on reconnect.
        if (state == ILink::State::Connected && vehicle.systemId() != 0)
            linkManager.send(makeStreamRequest(uint8_t(vehicle.systemId())));
    });
    QObject::connect(&linkManager, &LinkManager::reconnectScheduled, [&](int delayMs) {
        out << "[link] reconnecting in " << delayMs << " ms" << Qt::endl;
    });
    QObject::connect(&vehicle, &Vehicle::aliveChanged, [&](bool alive) {
        out << "[veh ] telemetry " << (alive ? "LIVE" : "LOST") << Qt::endl;
    });
    QObject::connect(&vehicle, &Vehicle::modeChanged, [&](const QString &mode, bool armed) {
        out << "[veh ] mode=" << mode << (armed ? " ARMED" : " disarmed") << Qt::endl;
    });
    QObject::connect(&vehicle, &Vehicle::attitudeChanged, [&](float roll, float pitch, float yaw) {
        out << QStringLiteral("[att ] roll=%1° pitch=%2° yaw=%3°")
                   .arg(double(roll), 7, 'f', 2)
                   .arg(double(pitch), 7, 'f', 2)
                   .arg(double(yaw), 7, 'f', 2)
            << Qt::endl;
    });
    QObject::connect(&vehicle, &Vehicle::statusTextReceived, [&](int severity, const QString &text) {
        out << "[msg ] (" << severity << ") " << text << Qt::endl;
    });

    linkManager.connectLink(config);
    out << "[poc ] target " << config.description() << " — Ctrl+C to quit" << Qt::endl;

    return QCoreApplication::exec();
}
