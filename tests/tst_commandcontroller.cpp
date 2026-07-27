#include <QSignalSpy>
#include <QTest>

#include "MavlinkTestUtils.h"
#include "core/CommandController.h"

using namespace kerkenez;
using namespace kerkenez::test;

class TestCommandController : public QObject
{
    Q_OBJECT

private slots:
    void sendsArmCommandToTheLockedVehicle()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle, 7);
        CommandController commands(&vehicle);
        SentMessages sent;
        connect(&commands, &CommandController::sendMessage, &sent, &SentMessages::capture);

        commands.arm(true);

        mavlink_message_t msg;
        QVERIFY(sent.lastOf(MAVLINK_MSG_ID_COMMAND_LONG, &msg));
        mavlink_command_long_t command;
        mavlink_msg_command_long_decode(&msg, &command);
        QCOMPARE(int(command.command), int(MAV_CMD_COMPONENT_ARM_DISARM));
        QCOMPARE(command.param1, 1.0f);
        QCOMPARE(int(command.target_system), 7);
        QVERIFY(commands.isBusy());
    }

    void reportsTheAcknowledgedResult()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle);
        CommandController commands(&vehicle);
        SentMessages sent;
        connect(&commands, &CommandController::sendMessage, &sent, &SentMessages::capture);
        QSignalSpy resultSpy(&commands, &CommandController::commandResult);

        commands.takeoff(25.0f);
        commands.handleMessage(makeCommandAck(MAV_CMD_NAV_TAKEOFF, MAV_RESULT_ACCEPTED));

        QCOMPARE(resultSpy.count(), 1);
        QCOMPARE(resultSpy.first().at(0).toInt(), int(MAV_CMD_NAV_TAKEOFF));
        QCOMPARE(resultSpy.first().at(1).toInt(), int(MAV_RESULT_ACCEPTED));
        QVERIFY(!commands.isBusy());
    }

    void ignoresAcksForOtherCommands()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle);
        CommandController commands(&vehicle);
        SentMessages sent;
        connect(&commands, &CommandController::sendMessage, &sent, &SentMessages::capture);
        QSignalSpy resultSpy(&commands, &CommandController::commandResult);

        commands.returnToLaunch();
        commands.handleMessage(makeCommandAck(MAV_CMD_NAV_LAND, MAV_RESULT_ACCEPTED));

        QCOMPARE(resultSpy.count(), 0);
        QVERIFY(commands.isBusy());
    }

    void retriesThenGivesUp()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle);
        CommandController commands(&vehicle);
        commands.setTimeoutMs(40);
        commands.setMaxAttempts(3);
        SentMessages sent;
        connect(&commands, &CommandController::sendMessage, &sent, &SentMessages::capture);
        QSignalSpy failedSpy(&commands, &CommandController::commandFailed);

        commands.land();

        QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, 2000);
        QCOMPARE(sent.countOf(MAVLINK_MSG_ID_COMMAND_LONG), 3);
        QVERIFY(!commands.isBusy());
    }

    void waitsWhileTheCommandIsInProgress()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle);
        CommandController commands(&vehicle);
        commands.setTimeoutMs(60);
        SentMessages sent;
        connect(&commands, &CommandController::sendMessage, &sent, &SentMessages::capture);
        QSignalSpy resultSpy(&commands, &CommandController::commandResult);

        commands.takeoff(10.0f);
        // Two progress reports must not restart the takeoff.
        for (int i = 0; i < 2; ++i) {
            QTest::qWait(30);
            commands.handleMessage(makeCommandAck(MAV_CMD_NAV_TAKEOFF, MAV_RESULT_IN_PROGRESS));
        }
        commands.handleMessage(makeCommandAck(MAV_CMD_NAV_TAKEOFF, MAV_RESULT_ACCEPTED));

        QCOMPARE(sent.countOf(MAVLINK_MSG_ID_COMMAND_LONG), 1);
        QCOMPARE(resultSpy.count(), 1);
    }

    void runsQueuedCommandsOneAtATime()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle);
        CommandController commands(&vehicle);
        SentMessages sent;
        connect(&commands, &CommandController::sendMessage, &sent, &SentMessages::capture);

        commands.arm(true);
        commands.takeoff(15.0f);
        QCOMPARE(sent.countOf(MAVLINK_MSG_ID_COMMAND_LONG), 1);

        commands.handleMessage(makeCommandAck(MAV_CMD_COMPONENT_ARM_DISARM, MAV_RESULT_ACCEPTED));
        QCOMPARE(sent.countOf(MAVLINK_MSG_ID_COMMAND_LONG), 2);

        mavlink_message_t msg;
        QVERIFY(sent.lastOf(MAVLINK_MSG_ID_COMMAND_LONG, &msg));
        mavlink_command_long_t command;
        mavlink_msg_command_long_decode(&msg, &command);
        QCOMPARE(int(command.command), int(MAV_CMD_NAV_TAKEOFF));
        QCOMPARE(command.param7, 15.0f);
    }

    void sendsGuidedTargetAtFullPrecision()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle);
        CommandController commands(&vehicle);
        SentMessages sent;
        connect(&commands, &CommandController::sendMessage, &sent, &SentMessages::capture);

        commands.flyTo(39.925533, 32.866287, 42.0f);

        mavlink_message_t msg;
        QVERIFY(sent.lastOf(MAVLINK_MSG_ID_SET_POSITION_TARGET_GLOBAL_INT, &msg));
        mavlink_set_position_target_global_int_t target;
        mavlink_msg_set_position_target_global_int_decode(&msg, &target);
        QCOMPARE(target.lat_int, 399255330);
        QCOMPARE(target.lon_int, 328662870);
        QCOMPARE(target.alt, 42.0f);
        QCOMPARE(int(target.coordinate_frame), int(MAV_FRAME_GLOBAL_RELATIVE_ALT_INT));
        // Fire-and-forget: no ack is expected for a setpoint.
        QVERIFY(!commands.isBusy());
    }
};

QTEST_GUILESS_MAIN(TestCommandController)
#include "tst_commandcontroller.moc"
