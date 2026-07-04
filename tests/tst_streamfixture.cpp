// Parses a real ArduPilot SITL byte stream recorded with tools/record_stream.py.

#include <QFile>
#include <QSet>
#include <QSignalSpy>
#include <QTest>

#include "core/MavlinkCodec.h"

using namespace kerkenez;

class TestStreamFixture : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QFile file(QStringLiteral(TEST_DATA_DIR "/sitl_stream.bin"));
        QVERIFY2(file.open(QIODevice::ReadOnly), "fixture missing — run tools/record_stream.py");
        m_stream = file.readAll();
        QVERIFY(m_stream.size() > 10000);
    }

    void parsesRecordedStreamWithoutErrors()
    {
        MavlinkCodec codec;
        QSet<int> seenMsgIds;
        connect(&codec, &MavlinkCodec::messageReceived, this,
                [&seenMsgIds](const mavlink_message_t &msg) { seenMsgIds.insert(int(msg.msgid)); });

        codec.feed(m_stream);

        QCOMPARE(codec.crcErrors(), quint64(0));
        QVERIFY2(codec.packetsReceived() > 100,
                 qPrintable(QStringLiteral("only %1 packets").arg(codec.packetsReceived())));
        QVERIFY(seenMsgIds.contains(MAVLINK_MSG_ID_HEARTBEAT));
        QVERIFY(seenMsgIds.contains(MAVLINK_MSG_ID_ATTITUDE));
        QVERIFY(seenMsgIds.contains(MAVLINK_MSG_ID_GLOBAL_POSITION_INT));

        m_wholeStreamCount = codec.packetsReceived();
    }

    void chunkedFeedMatchesWholeFeed()
    {
        MavlinkCodec codec;
        for (int offset = 0; offset < m_stream.size(); offset += 7)
            codec.feed(m_stream.mid(offset, 7));

        QCOMPARE(codec.crcErrors(), quint64(0));
        QCOMPARE(codec.packetsReceived(), m_wholeStreamCount);
    }

private:
    QByteArray m_stream;
    quint64 m_wholeStreamCount = 0;
};

QTEST_GUILESS_MAIN(TestStreamFixture)
#include "tst_streamfixture.moc"
