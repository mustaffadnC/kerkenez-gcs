#include "core/GcsMessages.h"

#include "core/MavlinkCodec.h"

namespace kerkenez {

QByteArray makeGcsHeartbeat()
{
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(kGcsSystemId, kGcsComponentId, &msg,
                               MAV_TYPE_GCS, MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_ACTIVE);
    return MavlinkCodec::pack(msg);
}

QByteArray makeStreamRequest(uint8_t targetSystem, uint16_t rateHz)
{
    mavlink_message_t msg;
    mavlink_msg_request_data_stream_pack(kGcsSystemId, kGcsComponentId, &msg,
                                         targetSystem, 0, MAV_DATA_STREAM_ALL, rateHz,
                                         /*start*/ 1);
    return MavlinkCodec::pack(msg);
}

} // namespace kerkenez
