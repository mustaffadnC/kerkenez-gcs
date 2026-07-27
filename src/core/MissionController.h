#pragma once

#include <QByteArray>
#include <QObject>
#include <QTimer>

#include "core/MissionItem.h"

namespace kerkenez {

class Vehicle;

// MAVLink mission protocol (https://mavlink.io/en/services/mission.html).
// Only the *_INT message variants are used: the float ones carry degrees in a
// float and lose about a metre of precision.
class MissionController : public QObject
{
    Q_OBJECT
public:
    enum class State { Idle, Uploading, Downloading, Clearing };
    Q_ENUM(State)

    explicit MissionController(Vehicle *vehicle, QObject *parent = nullptr);

    State state() const { return m_state; }
    void setTimeoutMs(int ms) { m_timeoutMs = ms; }
    void setMaxAttempts(int attempts) { m_maxAttempts = attempts; }

public slots:
    void handleMessage(const mavlink_message_t &msg);

    void upload(const MissionPlan &plan);
    void download();
    void clearOnVehicle();

signals:
    void sendMessage(const QByteArray &bytes);
    void progress(int done, int total);
    void uploadFinished();
    void downloadFinished(const kerkenez::MissionPlan &plan);
    void cleared();
    void failed(const QString &reason);

private:
    // seq 0 of an ArduPilot mission is the home position, not a waypoint.
    MissionPlan withHomeItem(const MissionPlan &plan) const;
    QByteArray packItem(uint16_t seq) const;
    QByteArray packCount() const;
    QByteArray packRequest(uint16_t seq) const;
    QByteArray packAck(uint8_t type) const;
    void sendTracked(const QByteArray &payload);
    void retransmit();
    void abort(const QString &reason);
    void reset();
    uint8_t target() const;

    Vehicle *m_vehicle;
    State m_state = State::Idle;

    MissionPlan m_outgoing;   // includes the home item at index 0
    MissionPlan m_incoming;   // includes the home item at index 0
    int m_expectedCount = 0;
    uint16_t m_requestedSeq = 0;

    QByteArray m_lastSent;
    int m_attempt = 0;
    QTimer m_timer;
    int m_timeoutMs = 1500;
    int m_maxAttempts = 3;
};

} // namespace kerkenez
