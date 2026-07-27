#pragma once

#include <QByteArray>
#include <QObject>
#include <QQueue>
#include <QTimer>

#include "core/MavlinkDefs.h"

namespace kerkenez {

class Vehicle;

// Sends COMMAND_LONG and matches the COMMAND_ACK that comes back. Commands are
// serialized one at a time; an unanswered command is retried before it is
// reported as failed, because a lost packet on a radio link is normal.
class CommandController : public QObject
{
    Q_OBJECT
public:
    explicit CommandController(Vehicle *vehicle, QObject *parent = nullptr);

    void setTimeoutMs(int ms) { m_timeoutMs = ms; }
    void setMaxAttempts(int attempts) { m_maxAttempts = attempts; }
    bool isBusy() const { return m_inFlight; }

public slots:
    void handleMessage(const mavlink_message_t &msg);

    void arm(bool arm);
    void takeoff(float altitudeMeters);
    void returnToLaunch();
    void land();
    void setFlightMode(uint32_t customMode);
    void startMission();
    // Guided "fly here" is a streamed setpoint, not an acknowledged command.
    void flyTo(double latitude, double longitude, float altitudeMeters);

signals:
    void sendMessage(const QByteArray &bytes);
    void commandSent(uint16_t command, int attempt);
    void commandResult(uint16_t command, int result);
    void commandFailed(uint16_t command, const QString &reason);

private:
    struct Pending
    {
        uint16_t command = 0;
        QByteArray payload;
        int attempt = 0;
    };

    void enqueue(uint16_t command, const QByteArray &payload);
    void transmit();
    void finishCurrent();

    Vehicle *m_vehicle;
    QQueue<Pending> m_queue;
    Pending m_current;
    bool m_inFlight = false;
    QTimer m_timer;
    int m_timeoutMs = 1500;
    int m_maxAttempts = 3;
};

} // namespace kerkenez
