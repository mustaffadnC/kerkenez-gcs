#include <QSignalSpy>
#include <QTest>

#include "core/Vehicle.h"

using namespace kerkenez;

namespace {

mavlink_message_t makeHeartbeat(uint8_t sysid, uint8_t type, uint32_t customMode, bool armed)
{
    mavlink_message_t msg;
    const uint8_t baseMode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED
        | (armed ? MAV_MODE_FLAG_SAFETY_ARMED : 0);
    mavlink_msg_heartbeat_pack(sysid, MAV_COMP_ID_AUTOPILOT1, &msg, type,
                               MAV_AUTOPILOT_ARDUPILOTMEGA, baseMode, customMode,
                               MAV_STATE_ACTIVE);
    return msg;
}

} // namespace

class TestVehicle : public QObject
{
    Q_OBJECT

private slots:
    void locksOntoFirstAutopilotAndReportsMode()
    {
        Vehicle vehicle;
        QSignalSpy firstHb(&vehicle, &Vehicle::firstHeartbeat);
        QSignalSpy mode(&vehicle, &Vehicle::modeChanged);

        vehicle.handleMessage(makeHeartbeat(1, MAV_TYPE_QUADROTOR, 4, true)); // Copter Guided

        QCOMPARE(firstHb.count(), 1);
        QCOMPARE(vehicle.systemId(), 1);
        QCOMPARE(mode.count(), 1);
        QCOMPARE(vehicle.modeName(), QStringLiteral("Guided"));
        QVERIFY(vehicle.armed());
        QVERIFY(vehicle.isAlive());
    }

    void ignoresGcsHeartbeatsAndOtherSystems()
    {
        Vehicle vehicle;
        QSignalSpy firstHb(&vehicle, &Vehicle::firstHeartbeat);

        mavlink_message_t gcs;
        mavlink_msg_heartbeat_pack(255, 190, &gcs, MAV_TYPE_GCS, MAV_AUTOPILOT_INVALID,
                                   0, 0, MAV_STATE_ACTIVE);
        vehicle.handleMessage(gcs);
        QCOMPARE(firstHb.count(), 0);

        vehicle.handleMessage(makeHeartbeat(7, MAV_TYPE_FIXED_WING, 15, false)); // Plane Guided
        QCOMPARE(vehicle.systemId(), 7);
        QCOMPARE(vehicle.modeName(), QStringLiteral("Guided"));

        // A second autopilot must not steal the lock.
        vehicle.handleMessage(makeHeartbeat(9, MAV_TYPE_QUADROTOR, 0, true));
        QCOMPARE(vehicle.systemId(), 7);
        QVERIFY(!vehicle.armed());
    }

    void convertsAttitudeToDegrees()
    {
        Vehicle vehicle;
        vehicle.handleMessage(makeHeartbeat(1, MAV_TYPE_QUADROTOR, 0, false));

        mavlink_message_t msg;
        mavlink_msg_attitude_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg, 0,
                                  0.1f, -0.2f, 1.0f, 0, 0, 0);
        QSignalSpy spy(&vehicle, &Vehicle::attitudeChanged);
        vehicle.handleMessage(msg);

        QCOMPARE(spy.count(), 1);
        QVERIFY(qAbs(vehicle.rollDeg() - 5.7296f) < 0.01f);
        QVERIFY(qAbs(vehicle.pitchDeg() + 11.4592f) < 0.01f);
    }

    void decodesStatusText()
    {
        Vehicle vehicle;
        vehicle.handleMessage(makeHeartbeat(1, MAV_TYPE_QUADROTOR, 0, false));

        mavlink_message_t msg;
        mavlink_msg_statustext_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg,
                                    MAV_SEVERITY_WARNING, "EKF ready", 0, 0);
        QSignalSpy spy(&vehicle, &Vehicle::statusTextReceived);
        vehicle.handleMessage(msg);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().at(0).toInt(), int(MAV_SEVERITY_WARNING));
        QCOMPARE(spy.first().at(1).toString(), QStringLiteral("EKF ready"));
    }

    void watchdogDropsAliveWithoutHeartbeats()
    {
        Vehicle vehicle;
        vehicle.setHeartbeatTimeoutMs(100);
        QSignalSpy alive(&vehicle, &Vehicle::aliveChanged);

        vehicle.handleMessage(makeHeartbeat(1, MAV_TYPE_QUADROTOR, 0, false));
        QVERIFY(vehicle.isAlive());
        QCOMPARE(alive.count(), 1);

        QTRY_VERIFY_WITH_TIMEOUT(!vehicle.isAlive(), 2000);
        QCOMPARE(alive.count(), 2);
        QCOMPARE(alive.last().at(0).toBool(), false);
    }
};

QTEST_GUILESS_MAIN(TestVehicle)
#include "tst_vehicle.moc"
