#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>

#include "core/MavlinkDefs.h"

namespace kerkenez {

// Central vehicle state, fed with decoded MAVLink messages. Locks onto the
// first autopilot heartbeat it sees and ignores other systems.
class Vehicle : public QObject
{
    Q_OBJECT
public:
    explicit Vehicle(QObject *parent = nullptr);

    // Identity — 0 until the first heartbeat.
    int systemId() const { return m_systemId; }
    bool isAlive() const { return m_alive; }

    bool armed() const { return m_armed; }
    QString modeName() const { return m_modeName; }
    uint8_t vehicleType() const { return m_vehicleType; }

    float rollDeg() const { return m_rollDeg; }
    float pitchDeg() const { return m_pitchDeg; }
    float yawDeg() const { return m_yawDeg; }

    double latitude() const { return m_latitude; }
    double longitude() const { return m_longitude; }
    float altitudeMsl() const { return m_altitudeMsl; }
    float altitudeRelative() const { return m_altitudeRelative; }
    float headingDeg() const { return m_headingDeg; }

    float airspeed() const { return m_airspeed; }
    float groundspeed() const { return m_groundspeed; }
    float climbRate() const { return m_climbRate; }
    int throttlePct() const { return m_throttlePct; }

    float batteryVoltage() const { return m_batteryVoltage; }
    float batteryCurrent() const { return m_batteryCurrent; }
    int batteryRemainingPct() const { return m_batteryRemainingPct; }

    int gpsFixType() const { return m_gpsFixType; }
    int gpsSatellites() const { return m_gpsSatellites; }

    bool hasHome() const { return m_hasHome; }
    double homeLatitude() const { return m_homeLatitude; }
    double homeLongitude() const { return m_homeLongitude; }

    // Heartbeat watchdog period (exposed for tests).
    void setHeartbeatTimeoutMs(int ms);

public slots:
    void handleMessage(const mavlink_message_t &msg);

signals:
    void firstHeartbeat(int systemId);
    void aliveChanged(bool alive);
    void modeChanged(const QString &modeName, bool armed);
    void attitudeChanged(float rollDeg, float pitchDeg, float yawDeg);
    void positionChanged(double lat, double lon, float altMsl, float altRel, float headingDeg);
    void vfrChanged(float airspeed, float groundspeed, float climbRate, int throttlePct);
    void batteryChanged(float voltage, float current, int remainingPct);
    void gpsChanged(int fixType, int satellites);
    void homeChanged(double lat, double lon);
    void statusTextReceived(int severity, const QString &text);

private:
    void handleHeartbeat(const mavlink_message_t &msg);

    int m_systemId = 0;
    bool m_alive = false;
    QTimer m_heartbeatWatchdog;

    bool m_armed = false;
    QString m_modeName;
    uint8_t m_vehicleType = MAV_TYPE_GENERIC;

    float m_rollDeg = 0, m_pitchDeg = 0, m_yawDeg = 0;
    double m_latitude = 0, m_longitude = 0;
    float m_altitudeMsl = 0, m_altitudeRelative = 0, m_headingDeg = 0;
    float m_airspeed = 0, m_groundspeed = 0, m_climbRate = 0;
    int m_throttlePct = 0;
    float m_batteryVoltage = 0, m_batteryCurrent = 0;
    int m_batteryRemainingPct = -1;
    int m_gpsFixType = 0, m_gpsSatellites = 0;
    bool m_hasHome = false;
    double m_homeLatitude = 0, m_homeLongitude = 0;
};

} // namespace kerkenez
