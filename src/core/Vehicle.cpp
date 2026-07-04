#include "core/Vehicle.h"

#include <QtMath>

#include "core/ApModes.h"

namespace kerkenez {

Vehicle::Vehicle(QObject *parent)
    : QObject(parent)
{
    m_heartbeatWatchdog.setSingleShot(true);
    m_heartbeatWatchdog.setInterval(3000);
    connect(&m_heartbeatWatchdog, &QTimer::timeout, this, [this] {
        if (m_alive) {
            m_alive = false;
            emit aliveChanged(false);
        }
    });
}

void Vehicle::setHeartbeatTimeoutMs(int ms)
{
    m_heartbeatWatchdog.setInterval(ms);
}

void Vehicle::handleMessage(const mavlink_message_t &msg)
{
    if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
        handleHeartbeat(msg);
        return;
    }
    if (m_systemId == 0 || msg.sysid != m_systemId)
        return;

    switch (msg.msgid) {
    case MAVLINK_MSG_ID_ATTITUDE: {
        mavlink_attitude_t att;
        mavlink_msg_attitude_decode(&msg, &att);
        m_rollDeg = float(qRadiansToDegrees(att.roll));
        m_pitchDeg = float(qRadiansToDegrees(att.pitch));
        m_yawDeg = float(qRadiansToDegrees(att.yaw));
        emit attitudeChanged(m_rollDeg, m_pitchDeg, m_yawDeg);
        break;
    }
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
        mavlink_global_position_int_t pos;
        mavlink_msg_global_position_int_decode(&msg, &pos);
        m_latitude = pos.lat / 1e7;
        m_longitude = pos.lon / 1e7;
        m_altitudeMsl = pos.alt / 1000.0f;
        m_altitudeRelative = pos.relative_alt / 1000.0f;
        if (pos.hdg != UINT16_MAX)
            m_headingDeg = pos.hdg / 100.0f;
        emit positionChanged(m_latitude, m_longitude, m_altitudeMsl, m_altitudeRelative,
                             m_headingDeg);
        break;
    }
    case MAVLINK_MSG_ID_VFR_HUD: {
        mavlink_vfr_hud_t vfr;
        mavlink_msg_vfr_hud_decode(&msg, &vfr);
        m_airspeed = vfr.airspeed;
        m_groundspeed = vfr.groundspeed;
        m_climbRate = vfr.climb;
        m_throttlePct = vfr.throttle;
        emit vfrChanged(m_airspeed, m_groundspeed, m_climbRate, m_throttlePct);
        break;
    }
    case MAVLINK_MSG_ID_SYS_STATUS: {
        mavlink_sys_status_t sys;
        mavlink_msg_sys_status_decode(&msg, &sys);
        m_batteryVoltage = sys.voltage_battery == UINT16_MAX ? 0.0f
                                                             : sys.voltage_battery / 1000.0f;
        m_batteryCurrent = sys.current_battery == -1 ? 0.0f : sys.current_battery / 100.0f;
        m_batteryRemainingPct = sys.battery_remaining;
        emit batteryChanged(m_batteryVoltage, m_batteryCurrent, m_batteryRemainingPct);
        break;
    }
    case MAVLINK_MSG_ID_GPS_RAW_INT: {
        mavlink_gps_raw_int_t gps;
        mavlink_msg_gps_raw_int_decode(&msg, &gps);
        m_gpsFixType = gps.fix_type;
        m_gpsSatellites = gps.satellites_visible;
        emit gpsChanged(m_gpsFixType, m_gpsSatellites);
        break;
    }
    case MAVLINK_MSG_ID_STATUSTEXT: {
        mavlink_statustext_t status;
        mavlink_msg_statustext_decode(&msg, &status);
        // text is not guaranteed to be null-terminated at full length
        char buffer[sizeof(status.text) + 1] = {};
        memcpy(buffer, status.text, sizeof(status.text));
        emit statusTextReceived(status.severity, QString::fromUtf8(buffer));
        break;
    }
    default:
        break;
    }
}

void Vehicle::handleHeartbeat(const mavlink_message_t &msg)
{
    mavlink_heartbeat_t hb;
    mavlink_msg_heartbeat_decode(&msg, &hb);

    // Ignore other ground stations and onboard peripherals.
    if (hb.type == MAV_TYPE_GCS || hb.autopilot == MAV_AUTOPILOT_INVALID)
        return;
    if (m_systemId == 0) {
        m_systemId = msg.sysid;
        emit firstHeartbeat(m_systemId);
    }
    if (msg.sysid != m_systemId)
        return;

    m_heartbeatWatchdog.start();
    if (!m_alive) {
        m_alive = true;
        emit aliveChanged(true);
    }

    m_vehicleType = hb.type;
    const bool armed = (hb.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) != 0;
    const QString mode = apModeName(hb.type, hb.custom_mode);
    if (armed != m_armed || mode != m_modeName) {
        m_armed = armed;
        m_modeName = mode;
        emit modeChanged(m_modeName, m_armed);
    }
}

} // namespace kerkenez
