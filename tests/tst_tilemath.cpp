#include <QTest>

#include "map/TileMath.h"

using namespace kerkenez;

class TestTileMath : public QObject
{
    Q_OBJECT

private slots:
    void mapsWorldCornersAndCenter()
    {
        for (const int zoom : {0, 1, 8, 16}) {
            const double n = TileMath::tileCount(zoom);
            QCOMPARE(TileMath::lonToTileX(-180.0, zoom), 0.0);
            QCOMPARE(TileMath::lonToTileX(180.0, zoom), n);
            QCOMPARE(TileMath::lonToTileX(0.0, zoom), n / 2);
            QVERIFY(qAbs(TileMath::latToTileY(0.0, zoom) - n / 2) < 1e-9);
        }
    }

    void roundTripsCoordinates()
    {
        const double coordinates[][2] = {
            {39.925533, 32.866287}, // Ankara — the SITL home
            {-33.8688, 151.2093},   // southern + eastern hemisphere
            {64.1466, -21.9426},    // high latitude, western hemisphere
            {0.0, 0.0},
        };
        for (const int zoom : {4, 12, 19}) {
            for (const auto &c : coordinates) {
                const double x = TileMath::lonToTileX(c[1], zoom);
                const double y = TileMath::latToTileY(c[0], zoom);
                QVERIFY(qAbs(TileMath::tileXToLon(x, zoom) - c[1]) < 1e-9);
                QVERIFY(qAbs(TileMath::tileYToLat(y, zoom) - c[0]) < 1e-9);
            }
        }
    }

    void clampsBeyondMercatorLimit()
    {
        const int zoom = 5;
        const double n = TileMath::tileCount(zoom);
        // Mercator is undefined at the poles; clamped values stay in range.
        const double north = TileMath::latToTileY(89.9, zoom);
        const double south = TileMath::latToTileY(-89.9, zoom);
        QVERIFY(north >= 0.0 && north < 1e-6);
        QVERIFY(south <= n && south > n - 1e-6);
    }

    void groundResolutionHalvesPerZoom()
    {
        // The textbook figure for 256 px tiles at the equator.
        QVERIFY(qAbs(TileMath::metersPerPixel(0.0, 0) - 156543.03) < 0.5);
        for (int zoom = 0; zoom < 18; ++zoom) {
            const double coarse = TileMath::metersPerPixel(0.0, zoom);
            const double fine = TileMath::metersPerPixel(0.0, zoom + 1);
            QVERIFY(qAbs(coarse / 2.0 - fine) < 1e-6);
        }
        // Meridians converge, so a pixel covers less ground away from the equator.
        QVERIFY(TileMath::metersPerPixel(60.0, 12) < TileMath::metersPerPixel(0.0, 12));
    }

    void wrapsColumnsAcrossAntimeridian()
    {
        QCOMPARE(TileMath::wrapTileX(-1, 3), 7);
        QCOMPARE(TileMath::wrapTileX(8, 3), 0);
        QCOMPARE(TileMath::wrapTileX(3, 3), 3);
    }

    void measuresGroundDistance()
    {
        // Ankara → Istanbul is about 350 km.
        const double meters = TileMath::haversineMeters(39.925533, 32.866287, 41.0082, 28.9784);
        QVERIFY2(qAbs(meters - 351000) < 20000, qPrintable(QString::number(meters)));
        QCOMPARE(TileMath::haversineMeters(10.0, 20.0, 10.0, 20.0), 0.0);
    }
};

QTEST_APPLESS_MAIN(TestTileMath)
#include "tst_tilemath.moc"
