#pragma once

#include <QList>
#include <QPair>
#include <QString>

#include "core/MavlinkDefs.h"

namespace kerkenez {

// ArduPilot flight-mode names live in HEARTBEAT.custom_mode and differ per
// vehicle family (Copter vs Plane vs Rover).
QString apModeName(uint8_t mavType, uint32_t customMode);

// Modes worth offering in a mode selector, in the order operators expect them.
QList<QPair<QString, uint32_t>> apSelectableModes(uint8_t mavType);

} // namespace kerkenez
