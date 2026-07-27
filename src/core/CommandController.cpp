#include "core/CommandController.h"

#include "core/GcsMessages.h"
#include "core/MavlinkCodec.h"
#include "core/Vehicle.h"

namespace kerkenez {

namespace {

QString resultText(int result)
{
    switch (result) {
    case MAV_RESULT_ACCEPTED: return QStringLiteral("accepted");
    case MAV_RESULT_TEMPORARILY_REJECTED: return QStringLiteral("temporarily rejected");
    case MAV_RESULT_DENIED: return QStringLiteral("denied");
    case MAV_RESULT_UNSUPPORTED: return QStringLiteral("unsupported");
    case MAV_RESULT_FAILED: return QStringLiteral("failed");
    default: return QStringLiteral("result %1").arg(result);
    }
}

} // namespace

CommandController::CommandController(Vehicle *vehicle, QObject *parent)
    : QObject(parent)
    , m_vehicle(vehicle)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        if (!m_inFlight)
            return;
        if (m_current.attempt < m_maxAttempts) {
            transmit();
            return;
        }
        const uint16_t command = m_current.command;
        finishCurrent();
        emit commandFailed(command, tr("no acknowledgement after %1 attempts").arg(m_maxAttempts));
    });
}

void CommandController::handleMessage(const mavlink_message_t &msg)
{
    if (msg.msgid != MAVLINK_MSG_ID_COMMAND_ACK || !m_inFlight)
        return;

    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&msg, &ack);
    if (ack.command != m_current.command)
        return;

    // The autopilot reports long-running commands as in progress; keep waiting
    // instead of resending, which would restart the operation.
    if (ack.result == MAV_RESULT_IN_PROGRESS) {
        m_timer.start(m_timeoutMs);
        return;
    }

    const uint16_t command = m_current.command;
    finishCurrent();
    emit commandResult(command, ack.result);
    if (ack.result != MAV_RESULT_ACCEPTED)
        emit commandFailed(command, resultText(ack.result));
}

void CommandController::enqueue(uint16_t command, const QByteArray &payload)
{
    m_queue.enqueue({command, payload, 0});
    if (!m_inFlight) {
        m_current = m_queue.dequeue();
        m_inFlight = true;
        transmit();
    }
}

void CommandController::transmit()
{
    ++m_current.attempt;
    emit sendMessage(m_current.payload);
    emit commandSent(m_current.command, m_current.attempt);
    m_timer.start(m_timeoutMs);
}

void CommandController::finishCurrent()
{
    m_timer.stop();
    if (m_queue.isEmpty()) {
        m_inFlight = false;
        m_current = {};
        return;
    }
    m_current = m_queue.dequeue();
    transmit();
}

namespace {

QByteArray packCommand(uint8_t targetSystem, uint16_t command, float p1 = 0, float p2 = 0,
                       float p3 = 0, float p4 = 0, float p5 = 0, float p6 = 0, float p7 = 0)
{
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(kGcsSystemId, kGcsComponentId, &msg, targetSystem,
                                  MAV_COMP_ID_AUTOPILOT1, command, /*confirmation*/ 0,
                                  p1, p2, p3, p4, p5, p6, p7);
    return MavlinkCodec::pack(msg);
}

} // namespace

void CommandController::arm(bool arm)
{
    const uint8_t target = uint8_t(m_vehicle->systemId());
    enqueue(MAV_CMD_COMPONENT_ARM_DISARM,
            packCommand(target, MAV_CMD_COMPONENT_ARM_DISARM, arm ? 1.0f : 0.0f));
}

void CommandController::takeoff(float altitudeMeters)
{
    const uint8_t target = uint8_t(m_vehicle->systemId());
    enqueue(MAV_CMD_NAV_TAKEOFF,
            packCommand(target, MAV_CMD_NAV_TAKEOFF, 0, 0, 0, 0, 0, 0, altitudeMeters));
}

void CommandController::returnToLaunch()
{
    const uint8_t target = uint8_t(m_vehicle->systemId());
    enqueue(MAV_CMD_NAV_RETURN_TO_LAUNCH, packCommand(target, MAV_CMD_NAV_RETURN_TO_LAUNCH));
}

void CommandController::land()
{
    const uint8_t target = uint8_t(m_vehicle->systemId());
    enqueue(MAV_CMD_NAV_LAND, packCommand(target, MAV_CMD_NAV_LAND));
}

void CommandController::setFlightMode(uint32_t customMode)
{
    const uint8_t target = uint8_t(m_vehicle->systemId());
    enqueue(MAV_CMD_DO_SET_MODE,
            packCommand(target, MAV_CMD_DO_SET_MODE, MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
                        float(customMode)));
}

void CommandController::startMission()
{
    const uint8_t target = uint8_t(m_vehicle->systemId());
    enqueue(MAV_CMD_MISSION_START, packCommand(target, MAV_CMD_MISSION_START));
}

void CommandController::flyTo(double latitude, double longitude, float altitudeMeters)
{
    // Position-only setpoint: ignore the velocity, acceleration and yaw fields.
    constexpr uint16_t kPositionOnly = 0x0DF8;

    mavlink_message_t msg;
    mavlink_msg_set_position_target_global_int_pack(
        kGcsSystemId, kGcsComponentId, &msg, /*time_boot_ms*/ 0, uint8_t(m_vehicle->systemId()),
        MAV_COMP_ID_AUTOPILOT1, MAV_FRAME_GLOBAL_RELATIVE_ALT_INT, kPositionOnly,
        int32_t(latitude * 1e7), int32_t(longitude * 1e7), altitudeMeters,
        0, 0, 0, 0, 0, 0, 0, 0);
    emit sendMessage(MavlinkCodec::pack(msg));
}

} // namespace kerkenez
