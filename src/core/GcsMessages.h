#pragma once

#include <QByteArray>

namespace kerkenez {

// Messages the ground station itself transmits.
inline constexpr uint8_t kGcsSystemId = 255;
inline constexpr uint8_t kGcsComponentId = 190; // MAV_COMP_ID_MISSIONPLANNER

QByteArray makeGcsHeartbeat();
QByteArray makeStreamRequest(uint8_t targetSystem, uint16_t rateHz = 10);

} // namespace kerkenez
