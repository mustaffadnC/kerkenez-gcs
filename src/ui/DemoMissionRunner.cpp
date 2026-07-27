#include "ui/DemoMissionRunner.h"

#include <cmath>

#include "core/ApModes.h"
#include "core/CommandController.h"
#include "core/MissionController.h"
#include "core/Vehicle.h"

namespace kerkenez {

DemoMissionRunner::DemoMissionRunner(Vehicle *vehicle, CommandController *commands,
                                     MissionController *mission, QObject *parent)
    : QObject(parent)
    , m_vehicle(vehicle)
    , m_commands(commands)
    , m_mission(mission)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &DemoMissionRunner::tick);

    connect(m_mission, &MissionController::uploadFinished, this, [this] {
        if (m_step != Step::Uploading)
            return;
        emit log(QStringLiteral("mission uploaded"));
        m_step = Step::SettingGuided;
        m_commands->setFlightMode(modeByName(QStringLiteral("Guided")));
    });
    connect(m_mission, &MissionController::failed, this, [this](const QString &reason) {
        emit log(QStringLiteral("mission upload failed: %1").arg(reason));
        m_step = Step::Waiting; // try again on the next tick
    });

    // Each accepted command only advances the step; tick() does the sending, so
    // a rejected command is simply retried on the next beat.
    connect(m_commands, &CommandController::commandResult, this,
            [this](uint16_t command, int result) {
                if (result != MAV_RESULT_ACCEPTED)
                    return;
                if (command == MAV_CMD_DO_SET_MODE && m_step == Step::SettingGuided) {
                    emit log(QStringLiteral("GUIDED accepted"));
                    m_step = Step::Arming;
                } else if (command == MAV_CMD_COMPONENT_ARM_DISARM && m_step == Step::Arming) {
                    emit log(QStringLiteral("armed"));
                    m_step = Step::TakingOff;
                } else if (command == MAV_CMD_NAV_TAKEOFF && m_step == Step::TakingOff) {
                    emit log(QStringLiteral("takeoff accepted"));
                    m_step = Step::Climbing;
                }
            });
    connect(m_commands, &CommandController::commandFailed, this,
            [this](uint16_t command, const QString &reason) {
                // Pre-arm checks keep failing until the EKF settles — expected.
                emit log(QStringLiteral("command %1 rejected: %2").arg(command).arg(reason));
            });
}

void DemoMissionRunner::start()
{
    emit log(QStringLiteral("demo mission armed and waiting for a position fix"));
    m_timer.start();
}

uint32_t DemoMissionRunner::modeByName(const QString &name) const
{
    for (const auto &mode : apSelectableModes(m_vehicle->vehicleType())) {
        if (mode.first.compare(name, Qt::CaseInsensitive) == 0)
            return mode.second;
    }
    return 0;
}

MissionPlan DemoMissionRunner::buildPlan() const
{
    const double lat = m_vehicle->homeLatitude();
    const double lon = m_vehicle->homeLongitude();
    // ~220 m legs; longitude degrees shrink with latitude.
    const double dLat = 0.0020;
    const double dLon = 0.0020 / std::cos(lat * 3.14159265358979 / 180.0);

    MissionPlan plan;
    MissionItem takeoff;
    takeoff.command = MAV_CMD_NAV_TAKEOFF;
    takeoff.latitude = lat;
    takeoff.longitude = lon;
    takeoff.altitude = m_takeoffAltitude;
    plan.append(takeoff);

    const double corners[4][2] = {
        {lat + dLat, lon},
        {lat + dLat, lon + dLon},
        {lat, lon + dLon},
        {lat - dLat * 0.4, lon + dLon * 0.4},
    };
    const float altitudes[4] = {40, 50, 45, 35};
    for (int i = 0; i < 4; ++i) {
        MissionItem item;
        item.command = MAV_CMD_NAV_WAYPOINT;
        item.latitude = corners[i][0];
        item.longitude = corners[i][1];
        item.altitude = altitudes[i];
        plan.append(item);
    }

    MissionItem rtl;
    rtl.command = MAV_CMD_NAV_RETURN_TO_LAUNCH;
    plan.append(rtl);
    return plan;
}

void DemoMissionRunner::tick()
{
    if (m_vehicle->systemId() == 0)
        return;

    switch (m_step) {
    case Step::Waiting: {
        if (m_vehicle->gpsFixType() < 3 || !m_vehicle->hasHome())
            return;
        const MissionPlan plan = buildPlan();
        emit planReady(plan);
        emit log(QStringLiteral("uploading %1 items").arg(plan.size()));
        m_step = Step::Uploading;
        m_mission->upload(plan);
        break;
    }
    case Step::SettingGuided:
        if (!m_commands->isBusy())
            m_commands->setFlightMode(modeByName(QStringLiteral("Guided")));
        break;
    case Step::Arming:
        if (!m_commands->isBusy())
            m_commands->arm(true);
        break;
    case Step::TakingOff:
        if (!m_commands->isBusy())
            m_commands->takeoff(m_takeoffAltitude);
        break;
    case Step::Climbing:
        if (m_vehicle->altitudeRelative() >= m_takeoffAltitude * 0.85f) {
            emit log(QStringLiteral("at altitude, switching to AUTO"));
            m_commands->setFlightMode(modeByName(QStringLiteral("Auto")));
            m_step = Step::Done;
            m_timer.stop();
        }
        break;
    case Step::Uploading:
    case Step::Done:
        break;
    }
}

} // namespace kerkenez
