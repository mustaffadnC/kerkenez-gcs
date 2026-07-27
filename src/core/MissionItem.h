#pragma once

#include <QVector>

#include "core/MavlinkDefs.h"

namespace kerkenez {

// One mission command. Altitudes are relative to home, matching
// MAV_FRAME_GLOBAL_RELATIVE_ALT_INT, which is what every item is sent with.
struct MissionItem
{
    uint16_t command = MAV_CMD_NAV_WAYPOINT;
    double latitude = 0;
    double longitude = 0;
    float altitude = 0;

    // Commands without a location (RTL, LAND at current position) still need a
    // row in the mission, but must not be drawn on the map.
    bool hasLocation() const
    {
        return command == MAV_CMD_NAV_WAYPOINT || command == MAV_CMD_NAV_LOITER_UNLIM
            || command == MAV_CMD_NAV_LOITER_TURNS || command == MAV_CMD_NAV_LOITER_TIME
            || command == MAV_CMD_NAV_SPLINE_WAYPOINT;
    }
};

using MissionPlan = QVector<MissionItem>;

QString missionCommandName(uint16_t command);

} // namespace kerkenez

Q_DECLARE_METATYPE(kerkenez::MissionItem)
Q_DECLARE_METATYPE(kerkenez::MissionPlan)
