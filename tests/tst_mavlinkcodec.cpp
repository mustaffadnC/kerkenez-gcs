#include <QSignalSpy>
#include <QTest>

#include "core/MavlinkCodec.h"

using namespace kerkenez;

namespace {

QByteArray heartbeatFrame()
{
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg,
                               MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_ARDUPILOTMEGA,
                               0, 0, MAV_STATE_ACTIVE);
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    const uint16_t length = mavlink_msg_to_send_buffer(buffer, &msg);
    return QByteArray(reinterpret_cast<const char *>(buffer), length);
}

} // namespace

class TestMavlinkCodec : public QObject
{
    Q_OBJECT

private slots:
    void decodesSingleHeartbeat()
    {
        MavlinkCodec codec;
        QSignalSpy spy(&codec, &MavlinkCodec::messageReceived);

        codec.feed(heartbeatFrame());

        QCOMPARE(spy.count(), 1);
        QCOMPARE(codec.packetsReceived(), quint64(1));
        QCOMPARE(codec.crcErrors(), quint64(0));

        const auto msg = spy.takeFirst().at(0).value<mavlink_message_t>();
        QCOMPARE(int(msg.msgid), int(MAVLINK_MSG_ID_HEARTBEAT));
        QCOMPARE(int(msg.sysid), 1);
    }

    void decodesFrameSplitAcrossChunks()
    {
        MavlinkCodec codec;
        QSignalSpy spy(&codec, &MavlinkCodec::messageReceived);

        const QByteArray frame = heartbeatFrame();
        const int split = frame.size() / 2;
        codec.feed(frame.left(split));
        QCOMPARE(spy.count(), 0); // incomplete frame must not emit
        codec.feed(frame.mid(split));

        QCOMPARE(spy.count(), 1);
    }

    void rejectsCorruptedCrc()
    {
        MavlinkCodec codec;
        QSignalSpy spy(&codec, &MavlinkCodec::messageReceived);

        QByteArray frame = heartbeatFrame();
        frame[frame.size() - 1] = ~frame[frame.size() - 1]; // flip last CRC byte
        codec.feed(frame);

        QCOMPARE(spy.count(), 0);
        QCOMPARE(codec.crcErrors(), quint64(1));
        QCOMPARE(codec.packetsReceived(), quint64(0));
    }

    void packProducesParseableBytes()
    {
        mavlink_message_t msg;
        mavlink_msg_heartbeat_pack(255, MAV_COMP_ID_MISSIONPLANNER, &msg,
                                   MAV_TYPE_GCS, MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_ACTIVE);

        MavlinkCodec codec;
        QSignalSpy spy(&codec, &MavlinkCodec::messageReceived);
        codec.feed(MavlinkCodec::pack(msg));

        QCOMPARE(spy.count(), 1);
        const auto decoded = spy.takeFirst().at(0).value<mavlink_message_t>();
        QCOMPARE(int(decoded.sysid), 255);
    }
};

QTEST_GUILESS_MAIN(TestMavlinkCodec)
#include "tst_mavlinkcodec.moc"
