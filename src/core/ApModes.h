#pragma once

#include <QString>

#include "core/MavlinkDefs.h"

namespace kerkenez {

// ArduPilot flight-mode names live in HEARTBEAT.custom_mode and differ per
// vehicle family (Copter vs Plane vs Rover).
QString apModeName(uint8_t mavType, uint32_t customMode);

} // namespace kerkenez
