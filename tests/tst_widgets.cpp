// Headless paint smoke tests: render every instrument into a QImage with
// normal and extreme values. Catches painter math crashes without a display.

#include <QImage>
#include <QTemporaryDir>
#include <QTest>

#include "core/Vehicle.h"
#include "map/TileCache.h"
#include "map/TileFetcher.h"
#include "map/TileMath.h"
#include "ui/AlertPanel.h"
#include "ui/CompassWidget.h"
#include "ui/MapWidget.h"
#include "ui/PfdWidget.h"
#include "ui/StatusPanel.h"

using namespace kerkenez;

namespace {

QImage renderWidget(QWidget &widget, const QSize &size)
{
    widget.resize(size);
    // Painting into a non-premultiplied ARGB32 image over a transparent fill
    // washes out the colors; an opaque RGB32 target keeps them exact.
    QImage image(size, QImage::Format_RGB32);
    image.fill(Qt::black);
    widget.render(&image);
    return image;
}

} // namespace

class TestWidgets : public QObject
{
    Q_OBJECT

private slots:
    void pfdRendersAcrossAttitudeRange()
    {
        PfdWidget pfd;
        const float attitudes[][3] = {
            {0, 0, 0}, {15, -5, 90}, {-45, 25, 359}, {180, 90, -180}, {-180, -90, 720},
        };
        for (const auto &att : attitudes) {
            pfd.setAttitude(att[0], att[1], att[2]);
            pfd.setSpeeds(12.5f, 14.0f, -2.5f, 55);
            pfd.setAltitudes(880.0f, 30.0f);
            const QImage image = renderWidget(pfd, {480, 360});
            QVERIFY(!image.isNull());
        }
    }

    void compassRendersFullCircle()
    {
        CompassWidget compass;
        for (float heading = -360; heading <= 720; heading += 45) {
            compass.setHeading(heading);
            const QImage image = renderWidget(compass, {170, 170});
            QVERIFY(!image.isNull());
        }
    }

    void panelsRenderWithLiveVehicle()
    {
        Vehicle vehicle;
        StatusPanel status(&vehicle);
        AlertPanel alerts(&vehicle);

        mavlink_message_t msg;
        mavlink_msg_heartbeat_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg, MAV_TYPE_QUADROTOR,
                                   MAV_AUTOPILOT_ARDUPILOTMEGA,
                                   MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_SAFETY_ARMED,
                                   4, MAV_STATE_ACTIVE);
        vehicle.handleMessage(msg);

        mavlink_msg_sys_status_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg, 0, 0, 0, 500, 12100,
                                    1500, 15 /* battery low */, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        vehicle.handleMessage(msg);

        mavlink_msg_statustext_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg, MAV_SEVERITY_ERROR,
                                    "EKF variance", 0, 0);
        vehicle.handleMessage(msg);

        QVERIFY(!renderWidget(status, {320, 160}).isNull());
        QVERIFY(!renderWidget(alerts, {320, 300}).isNull());
    }

    void mapDrawsCachedTilesWithoutNetwork()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        TileCache cache(dir.path());
        TileFetcher fetcher(&cache);
        fetcher.setOffline(true); // any network access here would be a bug

        constexpr int zoom = 16;
        constexpr double lat = 39.925533;
        constexpr double lon = 32.866287;
        const int centerX = int(TileMath::lonToTileX(lon, zoom));
        const int centerY = int(TileMath::latToTileY(lat, zoom));
        QImage tile(TileMath::kTileSize, TileMath::kTileSize, QImage::Format_ARGB32);
        tile.fill(Qt::magenta);
        for (int dx = -2; dx <= 2; ++dx)
            for (int dy = -2; dy <= 2; ++dy)
                cache.insert({zoom, centerX + dx, centerY + dy}, tile);

        MapWidget map(&cache, &fetcher);
        map.setZoom(zoom);
        map.setCenter(lat, lon);

        // Exact color, not just "renders": an overlay that leaks its brush
        // would tint the whole map.
        const QImage rendered = renderWidget(map, {400, 300});
        QCOMPARE(rendered.pixelColor(200, 150), QColor(Qt::magenta));
        QCOMPARE(rendered.pixelColor(20, 20), QColor(Qt::magenta));
    }

    void mapTracksVehicleAndBuildsATrail()
    {
        QTemporaryDir dir;
        TileCache cache(dir.path());
        TileFetcher fetcher(&cache);
        fetcher.setOffline(true);
        MapWidget map(&cache, &fetcher);

        // A pre-EKF (0, 0) fix must not drag the map into the Atlantic.
        map.setVehiclePosition(0.0, 0.0, 0, 0, 0);
        QCOMPARE(map.trailSize(), 0);

        map.setVehiclePosition(39.925533, 32.866287, 850, 10, 90);
        map.setVehiclePosition(39.935533, 32.876287, 850, 20, 180);
        QCOMPARE(map.trailSize(), 2);
        QVERIFY(qAbs(map.centerLatitude() - 39.935533) < 1e-9);

        // Panning releases the follow lock, so the map stops chasing.
        map.setFollowVehicle(false);
        const double frozen = map.centerLatitude();
        map.setVehiclePosition(39.945533, 32.886287, 850, 30, 270);
        QCOMPARE(map.centerLatitude(), frozen);
        QCOMPARE(map.trailSize(), 3);

        QVERIFY(!renderWidget(map, {400, 300}).isNull());
    }
};

QTEST_MAIN(TestWidgets)
#include "tst_widgets.moc"
