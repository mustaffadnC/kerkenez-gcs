#pragma once

#include <QObject>
#include <QTimer>

#include "core/MissionItem.h"

namespace kerkenez {

class CommandController;
class MissionController;
class Vehicle;

// Scripted end-to-end demo, driven entirely through the ground station's own
// controllers: build a plan → upload it → GUIDED → arm → take off → AUTO.
// Used for the README capture and as a manual integration check against SITL.
class DemoMissionRunner : public QObject
{
    Q_OBJECT
public:
    DemoMissionRunner(Vehicle *vehicle, CommandController *commands,
                      MissionController *mission, QObject *parent = nullptr);

    void start();

signals:
    void planReady(const kerkenez::MissionPlan &plan);
    void log(const QString &line);

private:
    enum class Step { Waiting, Uploading, SettingGuided, Arming, TakingOff, Climbing, Done };

    MissionPlan buildPlan() const;
    void tick();
    uint32_t modeByName(const QString &name) const;

    Vehicle *m_vehicle;
    CommandController *m_commands;
    MissionController *m_mission;
    Step m_step = Step::Waiting;
    QTimer m_timer;
    float m_takeoffAltitude = 30.0f;
};

} // namespace kerkenez
