#pragma once

#include <QObject>
#include <QVector>

#include "core/MavlinkCodec.h"
#include "core/Vehicle.h"

namespace kerkenez::test {

// Collects everything a controller transmits and decodes it back into
// messages, so tests assert on the wire format rather than on internals.
class SentMessages : public QObject
{
public:
    void capture(const QByteArray &bytes)
    {
        m_codec.feed(bytes);
    }

    SentMessages()
    {
        QObject::connect(&m_codec, &MavlinkCodec::messageReceived, this,
                         [this](const mavlink_message_t &msg) { m_messages.append(msg); });
    }

    int count() const { return m_messages.size(); }
    void clear() { m_messages.clear(); }
    const QVector<mavlink_message_t> &all() const { return m_messages; }

    int countOf(uint32_t msgid) const
    {
        int total = 0;
        for (const auto &msg : m_messages) {
            if (msg.msgid == msgid)
                ++total;
        }
        return total;
    }

    bool lastOf(uint32_t msgid, mavlink_message_t *out) const
    {
        for (int i = m_messages.size() - 1; i >= 0; --i) {
            if (m_messages.at(i).msgid == msgid) {
                if (out)
                    *out = m_messages.at(i);
                return true;
            }
        }
        return false;
    }

private:
    MavlinkCodec m_codec;
    QVector<mavlink_message_t> m_messages;
};

// Brings a Vehicle to the state controllers need: a known autopilot system id.
inline void giveVehicleHeartbeat(Vehicle &vehicle, uint8_t sysid = 1,
                                 uint8_t type = MAV_TYPE_QUADROTOR)
{
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(sysid, MAV_COMP_ID_AUTOPILOT1, &msg, type,
                               MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
                               0, MAV_STATE_ACTIVE);
    vehicle.handleMessage(msg);
}

inline mavlink_message_t makeCommandAck(uint16_t command, uint8_t result)
{
    mavlink_message_t msg;
    mavlink_msg_command_ack_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg, command, result,
                                 /*progress*/ 0, /*result_param2*/ 0, /*target_system*/ 255,
                                 /*target_component*/ 190);
    return msg;
}

} // namespace kerkenez::test
