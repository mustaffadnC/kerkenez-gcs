// Phase 0 proof of concept: connect to ArduPilot SITL over TCP, request the
// telemetry streams and print decoded HEARTBEAT / ATTITUDE / GLOBAL_POSITION_INT.
//
// Usage: poc_telemetry [host] [port]   (defaults: 127.0.0.1 5760)

#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

#include "comm/TcpLink.h"
#include "core/MavlinkCodec.h"

using namespace kerkenez;

namespace {

constexpr uint8_t kGcsSystemId = 255;
constexpr uint8_t kGcsComponentId = MAV_COMP_ID_MISSIONPLANNER;

QByteArray makeGcsHeartbeat()
{
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(kGcsSystemId, kGcsComponentId, &msg,
                               MAV_TYPE_GCS, MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_ACTIVE);
    return MavlinkCodec::pack(msg);
}

QByteArray makeStreamRequest(uint8_t targetSystem)
{
    mavlink_message_t msg;
    mavlink_msg_request_data_stream_pack(kGcsSystemId, kGcsComponentId, &msg,
                                         targetSystem, 0, MAV_DATA_STREAM_ALL,
                                         /*rate Hz*/ 4, /*start*/ 1);
    return MavlinkCodec::pack(msg);
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const QString host = argc > 1 ? argv[1] : QStringLiteral("127.0.0.1");
    const quint16 port = argc > 2 ? static_cast<quint16>(QString(argv[2]).toUShort()) : 5760;

    TcpLink link(host, port);
    MavlinkCodec codec;
    bool streamsRequested = false;

    QObject::connect(&link, &ILink::bytesReceived, &codec, &MavlinkCodec::feed);
    QObject::connect(&link, &ILink::stateChanged, [&](ILink::State state) {
        out << "[link] " << link.description() << " state="
            << (state == ILink::State::Connected     ? "connected"
                : state == ILink::State::Connecting  ? "connecting"
                                                     : "disconnected")
            << Qt::endl;
    });
    QObject::connect(&link, &ILink::errorOccurred, [&](const QString &message) {
        out << "[link] error: " << message << Qt::endl;
        QCoreApplication::exit(1);
    });

    QObject::connect(&codec, &MavlinkCodec::messageReceived, [&](const mavlink_message_t &msg) {
        switch (msg.msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT: {
            mavlink_heartbeat_t hb;
            mavlink_msg_heartbeat_decode(&msg, &hb);
            out << QStringLiteral("[hb  ] sys=%1 type=%2 autopilot=%3 armed=%4 custom_mode=%5")
                       .arg(msg.sysid)
                       .arg(hb.type)
                       .arg(hb.autopilot)
                       .arg((hb.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) ? 1 : 0)
                       .arg(hb.custom_mode)
                << Qt::endl;
            if (!streamsRequested && msg.sysid != kGcsSystemId) {
                streamsRequested = true;
                link.send(makeStreamRequest(static_cast<uint8_t>(msg.sysid)));
                out << "[poc ] requested telemetry streams at 4 Hz" << Qt::endl;
            }
            break;
        }
        case MAVLINK_MSG_ID_ATTITUDE: {
            mavlink_attitude_t att;
            mavlink_msg_attitude_decode(&msg, &att);
            out << QStringLiteral("[att ] roll=%1° pitch=%2° yaw=%3°")
                       .arg(qRadiansToDegrees(att.roll), 7, 'f', 2)
                       .arg(qRadiansToDegrees(att.pitch), 7, 'f', 2)
                       .arg(qRadiansToDegrees(att.yaw), 7, 'f', 2)
                << Qt::endl;
            break;
        }
        case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
            mavlink_global_position_int_t pos;
            mavlink_msg_global_position_int_decode(&msg, &pos);
            out << QStringLiteral("[pos ] lat=%1 lon=%2 alt=%3 m rel=%4 m")
                       .arg(pos.lat / 1e7, 0, 'f', 6)
                       .arg(pos.lon / 1e7, 0, 'f', 6)
                       .arg(pos.alt / 1000.0, 0, 'f', 1)
                       .arg(pos.relative_alt / 1000.0, 0, 'f', 1)
                << Qt::endl;
            break;
        }
        default:
            break;
        }
    });

    // GCS heartbeat at 1 Hz keeps the autopilot aware of us.
    QTimer heartbeatTimer;
    QObject::connect(&heartbeatTimer, &QTimer::timeout, [&] {
        link.send(makeGcsHeartbeat());
    });
    heartbeatTimer.start(1000);

    link.open();
    out << "[poc ] connecting to " << link.description() << " — Ctrl+C to quit" << Qt::endl;

    return QCoreApplication::exec();
}
