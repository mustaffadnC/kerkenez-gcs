#include <QSignalSpy>
#include <QTest>

#include <cstring>

#include "MavlinkTestUtils.h"
#include "core/ParamController.h"

using namespace kerkenez;
using namespace kerkenez::test;

namespace {

mavlink_message_t makeParamValue(const char *name, float value, uint16_t index, uint16_t count)
{
    mavlink_message_t msg;
    char id[16] = {};
    strncpy(id, name, 16);
    mavlink_msg_param_value_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg, id, value,
                                 MAV_PARAM_TYPE_REAL32, count, index);
    return msg;
}

} // namespace

class TestParamController : public QObject
{
    Q_OBJECT

private slots:
    void collectsTheWholeSet()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle);
        ParamController params(&vehicle);
        SentMessages sent;
        connect(&params, &ParamController::sendMessage, &sent, &SentMessages::capture);
        QSignalSpy finished(&params, &ParamController::refreshFinished);

        params.refresh();
        QCOMPARE(sent.countOf(MAVLINK_MSG_ID_PARAM_REQUEST_LIST), 1);
        QVERIFY(params.isLoading());

        params.handleMessage(makeParamValue("WPNAV_SPEED", 500.0f, 0, 3));
        params.handleMessage(makeParamValue("BATT_CAPACITY", 3300.0f, 1, 3));
        params.handleMessage(makeParamValue("RTL_ALT", 1500.0f, 2, 3));

        QCOMPARE(finished.count(), 1);
        QVERIFY(!params.isLoading());
        QCOMPARE(params.parameters().size(), 3);
        QCOMPARE(params.parameters().value(QStringLiteral("RTL_ALT")), 1500.0f);
    }

    void keepsFullLengthNames()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle);
        ParamController params(&vehicle);

        // param_id fills all 16 bytes with no terminator.
        params.handleMessage(makeParamValue("ABCDEFGHIJKLMNOP", 1.0f, 0, 1));

        QVERIFY(params.parameters().contains(QStringLiteral("ABCDEFGHIJKLMNOP")));
    }

    void refetchesIndicesThatNeverArrived()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle);
        ParamController params(&vehicle);
        params.setQuietPeriodMs(150);
        SentMessages sent;
        connect(&params, &ParamController::sendMessage, &sent, &SentMessages::capture);
        QSignalSpy finished(&params, &ParamController::refreshFinished);

        params.refresh();
        params.handleMessage(makeParamValue("FIRST", 1.0f, 0, 3));
        params.handleMessage(makeParamValue("THIRD", 3.0f, 2, 3)); // index 1 is lost

        // The retry repeats on a timer, so assert that it happened at all —
        // pinning an exact count races with however many passes have elapsed.
        QTRY_VERIFY_WITH_TIMEOUT(sent.countOf(MAVLINK_MSG_ID_PARAM_REQUEST_READ) >= 1, 5000);
        mavlink_message_t msg;
        QVERIFY(sent.lastOf(MAVLINK_MSG_ID_PARAM_REQUEST_READ, &msg));
        mavlink_param_request_read_t request;
        mavlink_msg_param_request_read_decode(&msg, &request);
        QCOMPARE(request.param_index, int16_t(1));

        params.handleMessage(makeParamValue("SECOND", 2.0f, 1, 3));
        QCOMPARE(finished.count(), 1);
        QCOMPARE(params.parameters().size(), 3);
    }

    void givesUpAfterRepeatedGaps()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle);
        ParamController params(&vehicle);
        params.setQuietPeriodMs(120);
        params.setMaxGapPasses(2);
        SentMessages sent;
        connect(&params, &ParamController::sendMessage, &sent, &SentMessages::capture);
        QSignalSpy failed(&params, &ParamController::failed);

        params.refresh();
        params.handleMessage(makeParamValue("ONLY", 1.0f, 0, 2));

        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 5000);
        QVERIFY(!params.isLoading());
    }

    void writesParameterWithItsOriginalType()
    {
        Vehicle vehicle;
        giveVehicleHeartbeat(vehicle);
        ParamController params(&vehicle);
        SentMessages sent;
        connect(&params, &ParamController::sendMessage, &sent, &SentMessages::capture);

        params.handleMessage(makeParamValue("RTL_ALT", 1500.0f, 0, 1));
        params.setParameter(QStringLiteral("RTL_ALT"), 2500.0f);

        mavlink_message_t msg;
        QVERIFY(sent.lastOf(MAVLINK_MSG_ID_PARAM_SET, &msg));
        mavlink_param_set_t set;
        mavlink_msg_param_set_decode(&msg, &set);
        QCOMPARE(set.param_value, 2500.0f);
        QCOMPARE(int(set.param_type), int(MAV_PARAM_TYPE_REAL32));
        QCOMPARE(QString::fromLatin1(set.param_id, 7), QStringLiteral("RTL_ALT"));
    }
};

QTEST_GUILESS_MAIN(TestParamController)
#include "tst_paramcontroller.moc"
