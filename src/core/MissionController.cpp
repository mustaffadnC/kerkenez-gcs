#include "core/MissionController.h"

#include "core/GcsMessages.h"
#include "core/MavlinkCodec.h"
#include "core/Vehicle.h"

namespace kerkenez {

MissionController::MissionController(Vehicle *vehicle, QObject *parent)
    : QObject(parent)
    , m_vehicle(vehicle)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &MissionController::retransmit);
}

uint8_t MissionController::target() const
{
    return uint8_t(m_vehicle->systemId());
}

MissionPlan MissionController::withHomeItem(const MissionPlan &plan) const
{
    MissionItem home;
    home.command = MAV_CMD_NAV_WAYPOINT;
    if (m_vehicle->hasHome()) {
        home.latitude = m_vehicle->homeLatitude();
        home.longitude = m_vehicle->homeLongitude();
    } else if (!plan.isEmpty()) {
        home.latitude = plan.first().latitude;
        home.longitude = plan.first().longitude;
    }

    MissionPlan full;
    full.reserve(plan.size() + 1);
    full.append(home);
    full.append(plan);
    return full;
}

QByteArray MissionController::packCount() const
{
    mavlink_message_t msg;
    mavlink_msg_mission_count_pack(kGcsSystemId, kGcsComponentId, &msg, target(),
                                   MAV_COMP_ID_AUTOPILOT1, uint16_t(m_outgoing.size()),
                                   MAV_MISSION_TYPE_MISSION, /*opaque_id*/ 0);
    return MavlinkCodec::pack(msg);
}

QByteArray MissionController::packItem(uint16_t seq) const
{
    const MissionItem &item = m_outgoing.at(seq);
    mavlink_message_t msg;
    mavlink_msg_mission_item_int_pack(
        kGcsSystemId, kGcsComponentId, &msg, target(), MAV_COMP_ID_AUTOPILOT1, seq,
        MAV_FRAME_GLOBAL_RELATIVE_ALT_INT, item.command, /*current*/ seq == 0 ? 1 : 0,
        /*autocontinue*/ 1, 0, 0, 0, 0, int32_t(item.latitude * 1e7),
        int32_t(item.longitude * 1e7), item.altitude, MAV_MISSION_TYPE_MISSION);
    return MavlinkCodec::pack(msg);
}

QByteArray MissionController::packRequest(uint16_t seq) const
{
    mavlink_message_t msg;
    mavlink_msg_mission_request_int_pack(kGcsSystemId, kGcsComponentId, &msg, target(),
                                         MAV_COMP_ID_AUTOPILOT1, seq,
                                         MAV_MISSION_TYPE_MISSION);
    return MavlinkCodec::pack(msg);
}

QByteArray MissionController::packAck(uint8_t type) const
{
    mavlink_message_t msg;
    mavlink_msg_mission_ack_pack(kGcsSystemId, kGcsComponentId, &msg, target(),
                                 MAV_COMP_ID_AUTOPILOT1, type, MAV_MISSION_TYPE_MISSION,
                                 /*opaque_id*/ 0);
    return MavlinkCodec::pack(msg);
}

void MissionController::sendTracked(const QByteArray &payload)
{
    m_lastSent = payload;
    m_attempt = 1;
    emit sendMessage(payload);
    m_timer.start(m_timeoutMs);
}

void MissionController::retransmit()
{
    if (m_state == State::Idle)
        return;
    if (m_attempt >= m_maxAttempts) {
        abort(tr("no response after %1 attempts").arg(m_maxAttempts));
        return;
    }
    ++m_attempt;
    emit sendMessage(m_lastSent);
    m_timer.start(m_timeoutMs);
}

void MissionController::abort(const QString &reason)
{
    reset();
    emit failed(reason);
}

void MissionController::reset()
{
    m_timer.stop();
    m_state = State::Idle;
    m_lastSent.clear();
    m_attempt = 0;
    m_outgoing.clear();
    m_incoming.clear();
    m_expectedCount = 0;
}

void MissionController::upload(const MissionPlan &plan)
{
    if (m_state != State::Idle) {
        emit failed(tr("mission transfer already in progress"));
        return;
    }
    if (plan.isEmpty()) {
        clearOnVehicle();
        return;
    }
    m_outgoing = withHomeItem(plan);
    m_state = State::Uploading;
    emit progress(0, m_outgoing.size());
    sendTracked(packCount());
}

void MissionController::download()
{
    if (m_state != State::Idle) {
        emit failed(tr("mission transfer already in progress"));
        return;
    }
    m_state = State::Downloading;
    m_incoming.clear();

    mavlink_message_t msg;
    mavlink_msg_mission_request_list_pack(kGcsSystemId, kGcsComponentId, &msg, target(),
                                          MAV_COMP_ID_AUTOPILOT1, MAV_MISSION_TYPE_MISSION);
    sendTracked(MavlinkCodec::pack(msg));
}

void MissionController::clearOnVehicle()
{
    if (m_state != State::Idle) {
        emit failed(tr("mission transfer already in progress"));
        return;
    }
    m_state = State::Clearing;

    mavlink_message_t msg;
    mavlink_msg_mission_clear_all_pack(kGcsSystemId, kGcsComponentId, &msg, target(),
                                       MAV_COMP_ID_AUTOPILOT1, MAV_MISSION_TYPE_MISSION);
    sendTracked(MavlinkCodec::pack(msg));
}

void MissionController::handleMessage(const mavlink_message_t &msg)
{
    if (m_state == State::Idle)
        return;

    switch (msg.msgid) {
    // Autopilots ask with either the INT or the legacy request; both mean the
    // same thing and are answered with MISSION_ITEM_INT.
    case MAVLINK_MSG_ID_MISSION_REQUEST_INT:
    case MAVLINK_MSG_ID_MISSION_REQUEST: {
        if (m_state != State::Uploading)
            return;
        uint16_t seq = 0;
        if (msg.msgid == MAVLINK_MSG_ID_MISSION_REQUEST_INT) {
            mavlink_mission_request_int_t request;
            mavlink_msg_mission_request_int_decode(&msg, &request);
            seq = request.seq;
        } else {
            mavlink_mission_request_t request;
            mavlink_msg_mission_request_decode(&msg, &request);
            seq = request.seq;
        }
        if (seq >= m_outgoing.size()) {
            abort(tr("vehicle asked for item %1 of %2").arg(seq).arg(m_outgoing.size()));
            return;
        }
        emit progress(seq + 1, m_outgoing.size());
        sendTracked(packItem(seq));
        break;
    }

    case MAVLINK_MSG_ID_MISSION_COUNT: {
        if (m_state != State::Downloading)
            return;
        mavlink_mission_count_t count;
        mavlink_msg_mission_count_decode(&msg, &count);
        m_expectedCount = count.count;
        if (m_expectedCount == 0) {
            m_timer.stop();
            emit sendMessage(packAck(MAV_MISSION_ACCEPTED));
            reset();
            emit downloadFinished({});
            return;
        }
        m_requestedSeq = 0;
        emit progress(0, m_expectedCount);
        sendTracked(packRequest(m_requestedSeq));
        break;
    }

    case MAVLINK_MSG_ID_MISSION_ITEM_INT: {
        if (m_state != State::Downloading)
            return;
        mavlink_mission_item_int_t item;
        mavlink_msg_mission_item_int_decode(&msg, &item);
        if (item.seq != m_requestedSeq)
            return; // duplicate or out of order; the pending request still stands

        MissionItem parsed;
        parsed.command = item.command;
        parsed.latitude = item.x / 1e7;
        parsed.longitude = item.y / 1e7;
        parsed.altitude = item.z;
        m_incoming.append(parsed);
        emit progress(m_incoming.size(), m_expectedCount);

        if (m_incoming.size() >= m_expectedCount) {
            m_timer.stop();
            emit sendMessage(packAck(MAV_MISSION_ACCEPTED));
            MissionPlan plan = m_incoming;
            if (!plan.isEmpty())
                plan.removeFirst(); // drop the home item
            reset();
            emit downloadFinished(plan);
            return;
        }
        ++m_requestedSeq;
        sendTracked(packRequest(m_requestedSeq));
        break;
    }

    case MAVLINK_MSG_ID_MISSION_ACK: {
        mavlink_mission_ack_t ack;
        mavlink_msg_mission_ack_decode(&msg, &ack);
        const State finishing = m_state;
        if (finishing == State::Downloading)
            return; // we are the ones acking a download

        if (ack.type != MAV_MISSION_ACCEPTED) {
            abort(tr("vehicle rejected the mission (type %1)").arg(ack.type));
            return;
        }
        const int total = m_outgoing.size();
        reset();
        if (finishing == State::Uploading) {
            emit progress(total, total);
            emit uploadFinished();
        } else {
            emit cleared();
        }
        break;
    }

    default:
        break;
    }
}

} // namespace kerkenez
