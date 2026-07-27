#include <QSignalSpy>
#include <QTest>

#include "MavlinkTestUtils.h"
#include "core/MissionController.h"

using namespace kerkenez;
using namespace kerkenez::test;

namespace {

constexpr uint8_t kVehicleSysId = 1;

mavlink_message_t makeRequestInt(uint16_t seq)
{
    mavlink_message_t msg;
    mavlink_msg_mission_request_int_pack(kVehicleSysId, MAV_COMP_ID_AUTOPILOT1, &msg, 255, 190,
                                         seq, MAV_MISSION_TYPE_MISSION);
    return msg;
}

mavlink_message_t makeMissionAck(uint8_t type)
{
    mavlink_message_t msg;
    mavlink_msg_mission_ack_pack(kVehicleSysId, MAV_COMP_ID_AUTOPILOT1, &msg, 255, 190, type,
                                 MAV_MISSION_TYPE_MISSION, 0);
    return msg;
}

mavlink_message_t makeMissionCount(uint16_t count)
{
    mavlink_message_t msg;
    mavlink_msg_mission_count_pack(kVehicleSysId, MAV_COMP_ID_AUTOPILOT1, &msg, 255, 190, count,
                                   MAV_MISSION_TYPE_MISSION, 0);
    return msg;
}

mavlink_message_t makeItemInt(uint16_t seq, uint16_t command, double lat, double lon, float alt)
{
    mavlink_message_t msg;
    mavlink_msg_mission_item_int_pack(kVehicleSysId, MAV_COMP_ID_AUTOPILOT1, &msg, 255, 190, seq,
                                      MAV_FRAME_GLOBAL_RELATIVE_ALT_INT, command, 0, 1, 0, 0, 0,
                                      0, int32_t(lat * 1e7), int32_t(lon * 1e7), alt,
                                      MAV_MISSION_TYPE_MISSION);
    return msg;
}

MissionPlan twoWaypointPlan()
{
    MissionPlan plan;
    MissionItem first;
    first.command = MAV_CMD_NAV_WAYPOINT;
    first.latitude = 39.925533;
    first.longitude = 32.866287;
    first.altitude = 40;
    MissionItem second = first;
    second.latitude = 39.935533;
    second.altitude = 55;
    plan << first << second;
    return plan;
}

} // namespace

class TestMissionController : public QObject
{
    Q_OBJECT

private slots:
    void uploadsHomeThenItemsAndFinishesOnAck()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle, kVehicleSysId);
        MissionController mission(&vehicle);
        SentMessages sent;
        connect(&mission, &MissionController::sendMessage, &sent, &SentMessages::capture);
        QSignalSpy finished(&mission, &MissionController::uploadFinished);

        const MissionPlan plan = twoWaypointPlan();
        mission.upload(plan);

        // seq 0 is home, so the vehicle is told about three items, not two.
        mavlink_message_t msg;
        QVERIFY(sent.lastOf(MAVLINK_MSG_ID_MISSION_COUNT, &msg));
        mavlink_mission_count_t count;
        mavlink_msg_mission_count_decode(&msg, &count);
        QCOMPARE(int(count.count), 3);

        for (uint16_t seq = 0; seq < 3; ++seq)
            mission.handleMessage(makeRequestInt(seq));
        QCOMPARE(sent.countOf(MAVLINK_MSG_ID_MISSION_ITEM_INT), 3);

        QVERIFY(sent.lastOf(MAVLINK_MSG_ID_MISSION_ITEM_INT, &msg));
        mavlink_mission_item_int_t item;
        mavlink_msg_mission_item_int_decode(&msg, &item);
        QCOMPARE(int(item.seq), 2);
        QCOMPARE(item.x, 399355330);
        QCOMPARE(item.y, 328662870);
        QCOMPARE(item.z, 55.0f);
        QCOMPARE(int(item.frame), int(MAV_FRAME_GLOBAL_RELATIVE_ALT_INT));
        QCOMPARE(int(item.command), int(MAV_CMD_NAV_WAYPOINT));

        mission.handleMessage(makeMissionAck(MAV_MISSION_ACCEPTED));
        QCOMPARE(finished.count(), 1);
        QCOMPARE(mission.state(), MissionController::State::Idle);
    }

    void answersTheLegacyRequestVariantToo()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle, kVehicleSysId);
        MissionController mission(&vehicle);
        SentMessages sent;
        connect(&mission, &MissionController::sendMessage, &sent, &SentMessages::capture);

        mission.upload(twoWaypointPlan());

        mavlink_message_t legacy;
        mavlink_msg_mission_request_pack(kVehicleSysId, MAV_COMP_ID_AUTOPILOT1, &legacy, 255, 190,
                                         1, MAV_MISSION_TYPE_MISSION);
        mission.handleMessage(legacy);

        mavlink_message_t msg;
        QVERIFY(sent.lastOf(MAVLINK_MSG_ID_MISSION_ITEM_INT, &msg));
        mavlink_mission_item_int_t item;
        mavlink_msg_mission_item_int_decode(&msg, &item);
        QCOMPARE(int(item.seq), 1);
    }

    void reportsRejectionFromTheVehicle()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle, kVehicleSysId);
        MissionController mission(&vehicle);
        SentMessages sent;
        connect(&mission, &MissionController::sendMessage, &sent, &SentMessages::capture);
        QSignalSpy failed(&mission, &MissionController::failed);

        mission.upload(twoWaypointPlan());
        mission.handleMessage(makeMissionAck(MAV_MISSION_ERROR));

        QCOMPARE(failed.count(), 1);
        QCOMPARE(mission.state(), MissionController::State::Idle);
    }

    void retransmitsAndEventuallyFails()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle, kVehicleSysId);
        MissionController mission(&vehicle);
        mission.setTimeoutMs(40);
        mission.setMaxAttempts(3);
        SentMessages sent;
        connect(&mission, &MissionController::sendMessage, &sent, &SentMessages::capture);
        QSignalSpy failed(&mission, &MissionController::failed);

        mission.upload(twoWaypointPlan());

        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 2000);
        QCOMPARE(sent.countOf(MAVLINK_MSG_ID_MISSION_COUNT), 3);
    }

    void downloadsAndDropsTheHomeItem()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle, kVehicleSysId);
        MissionController mission(&vehicle);
        SentMessages sent;
        connect(&mission, &MissionController::sendMessage, &sent, &SentMessages::capture);
        QSignalSpy finished(&mission, &MissionController::downloadFinished);

        mission.download();
        QCOMPARE(sent.countOf(MAVLINK_MSG_ID_MISSION_REQUEST_LIST), 1);

        mission.handleMessage(makeMissionCount(3));
        mission.handleMessage(makeItemInt(0, MAV_CMD_NAV_WAYPOINT, 39.9, 32.8, 0));
        mission.handleMessage(makeItemInt(1, MAV_CMD_NAV_TAKEOFF, 39.9, 32.8, 30));
        mission.handleMessage(makeItemInt(2, MAV_CMD_NAV_WAYPOINT, 39.945678, 32.812345, 60));

        QCOMPARE(finished.count(), 1);
        const auto plan = finished.first().at(0).value<MissionPlan>();
        QCOMPARE(plan.size(), 2);
        QCOMPARE(int(plan.at(0).command), int(MAV_CMD_NAV_TAKEOFF));
        QVERIFY(qAbs(plan.at(1).latitude - 39.945678) < 1e-7);
        QVERIFY(qAbs(plan.at(1).longitude - 32.812345) < 1e-7);
        QCOMPARE(plan.at(1).altitude, 60.0f);
        QCOMPARE(sent.countOf(MAVLINK_MSG_ID_MISSION_ACK), 1);
    }

    void ignoresOutOfOrderItemsDuringDownload()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle, kVehicleSysId);
        MissionController mission(&vehicle);
        SentMessages sent;
        connect(&mission, &MissionController::sendMessage, &sent, &SentMessages::capture);
        QSignalSpy finished(&mission, &MissionController::downloadFinished);

        mission.download();
        mission.handleMessage(makeMissionCount(2));
        mission.handleMessage(makeItemInt(1, MAV_CMD_NAV_WAYPOINT, 39.9, 32.8, 10)); // unexpected
        QCOMPARE(finished.count(), 0);

        mission.handleMessage(makeItemInt(0, MAV_CMD_NAV_WAYPOINT, 39.9, 32.8, 0));
        mission.handleMessage(makeItemInt(1, MAV_CMD_NAV_WAYPOINT, 39.9, 32.8, 10));
        QCOMPARE(finished.count(), 1);
    }
};

QTEST_GUILESS_MAIN(TestMissionController)
#include "tst_missioncontroller.moc"
