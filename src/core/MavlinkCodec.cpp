#include "core/MavlinkCodec.h"

namespace kerkenez {

MavlinkCodec::MavlinkCodec(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<mavlink_message_t>("mavlink_message_t");
}

void MavlinkCodec::feed(const QByteArray &bytes)
{
    for (const char c : bytes) {
        mavlink_message_t msg;
        mavlink_status_t status;
        const uint8_t framing = mavlink_frame_char_buffer(
            &m_rxBuffer, &m_rxBufferStatus, static_cast<uint8_t>(c), &msg, &status);

        if (framing == MAVLINK_FRAMING_BAD_CRC || framing == MAVLINK_FRAMING_BAD_SIGNATURE) {
            ++m_crcErrors;
            continue;
        }
        if (framing != MAVLINK_FRAMING_OK)
            continue;

        ++m_packetsReceived;
        if (m_lastSeq >= 0) {
            const int expected = (m_lastSeq + 1) & 0xFF;
            if (msg.seq != expected)
                m_packetsLost += (msg.seq - expected) & 0xFF;
        }
        m_lastSeq = msg.seq;

        emit messageReceived(msg);
    }
}

void MavlinkCodec::reset()
{
    m_rxBuffer = {};
    m_rxBufferStatus = {};
    m_packetsReceived = 0;
    m_crcErrors = 0;
    m_packetsLost = 0;
    m_lastSeq = -1;
}

QByteArray MavlinkCodec::pack(const mavlink_message_t &msg)
{
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    const uint16_t length = mavlink_msg_to_send_buffer(buffer, &msg);
    return QByteArray(reinterpret_cast<const char *>(buffer), length);
}

} // namespace kerkenez
