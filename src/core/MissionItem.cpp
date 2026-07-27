#include "core/MissionItem.h"

#include <QString>

namespace kerkenez {

QString missionCommandName(uint16_t command)
{
    switch (command) {
    case MAV_CMD_NAV_WAYPOINT: return QStringLiteral("Waypoint");
    case MAV_CMD_NAV_TAKEOFF: return QStringLiteral("Takeoff");
    case MAV_CMD_NAV_LAND: return QStringLiteral("Land");
    case MAV_CMD_NAV_RETURN_TO_LAUNCH: return QStringLiteral("Return to launch");
    case MAV_CMD_NAV_LOITER_UNLIM: return QStringLiteral("Loiter");
    case MAV_CMD_NAV_LOITER_TIME: return QStringLiteral("Loiter (time)");
    case MAV_CMD_NAV_LOITER_TURNS: return QStringLiteral("Loiter (turns)");
    case MAV_CMD_NAV_SPLINE_WAYPOINT: return QStringLiteral("Spline waypoint");
    default: return QStringLiteral("Cmd %1").arg(command);
    }
}

} // namespace kerkenez
