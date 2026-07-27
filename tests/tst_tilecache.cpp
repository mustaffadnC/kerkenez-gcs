#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include "map/TileCache.h"

using namespace kerkenez;

namespace {

QImage solidTile(QColor color)
{
    QImage image(256, 256, QImage::Format_ARGB32);
    image.fill(color);
    return image;
}

} // namespace

class TestTileCache : public QObject
{
    Q_OBJECT

private slots:
    void storesAndReturnsTiles()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        TileCache cache(dir.path());

        const TileId id{16, 38712, 24651};
        QVERIFY(!cache.get(id, nullptr));
        QVERIFY(!cache.containsOnDisk(id));

        cache.insert(id, solidTile(Qt::magenta));

        QImage out;
        QVERIFY(cache.get(id, &out));
        QCOMPARE(out.size(), QSize(256, 256));
        QCOMPARE(out.pixelColor(10, 10), QColor(Qt::magenta));
    }

    void writesTilesToADiskTree()
    {
        QTemporaryDir dir;
        TileCache cache(dir.path());
        const TileId id{12, 2421, 1551};
        cache.insert(id, solidTile(Qt::blue));

        QVERIFY(cache.containsOnDisk(id));
        QVERIFY(QFileInfo::exists(dir.path() + QStringLiteral("/12/2421/1551.png")));
        QVERIFY(cache.diskUsageBytes() > 0);
    }

    void servesTilesFromDiskInAFreshInstance()
    {
        // This is what offline mode relies on: a new session with no network
        // still renders everything that was viewed before.
        QTemporaryDir dir;
        const TileId id{14, 9678, 6162};
        {
            TileCache warm(dir.path());
            warm.insert(id, solidTile(Qt::darkGreen));
        }

        TileCache cold(dir.path());
        QImage out;
        QVERIFY(cold.get(id, &out));
        QCOMPARE(out.pixelColor(128, 128), QColor(Qt::darkGreen));
    }

    void keepsUpstreamBytesWhenProvided()
    {
        QTemporaryDir dir;
        TileCache cache(dir.path());
        const TileId id{5, 3, 7};

        QImage image = solidTile(Qt::red);
        QByteArray encoded;
        {
            QBuffer buffer(&encoded);
            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "PNG");
        }
        cache.insert(id, image, encoded);

        QFile file(dir.path() + QStringLiteral("/5/3/7.png"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), encoded);
    }
};

QTEST_GUILESS_MAIN(TestTileCache)
#include "tst_tilecache.moc"
