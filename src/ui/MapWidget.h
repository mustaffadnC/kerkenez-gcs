#pragma once

#include <QPoint>
#include <QVector>
#include <QWidget>

#include "map/TileId.h"

namespace kerkenez {

class TileCache;
class TileFetcher;

// Slippy map drawn with QPainter: OSM tiles from the cache, vehicle icon,
// flight trail and home marker on top. No web engine involved, so the same
// widget works offline from the disk cache.
class MapWidget : public QWidget
{
    Q_OBJECT
public:
    MapWidget(TileCache *cache, TileFetcher *fetcher, QWidget *parent = nullptr);

    QSize minimumSizeHint() const override { return {320, 240}; }

    double centerLatitude() const { return m_centerLat; }
    double centerLongitude() const { return m_centerLon; }
    int zoom() const { return m_zoom; }
    bool followsVehicle() const { return m_followVehicle; }
    int trailSize() const { return m_trail.size(); }

    void setCenter(double lat, double lon);
    void setZoom(int zoom);

public slots:
    void setVehiclePosition(double lat, double lon, float altMsl, float altRel, float headingDeg);
    void setHomePosition(double lat, double lon);
    void setFollowVehicle(bool follow);
    void clearTrail();
    void centerOnVehicle();

signals:
    void followVehicleChanged(bool follow);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QPointF geoToScreen(double lat, double lon) const;
    void screenToGeo(const QPointF &point, double *lat, double *lon) const;
    void drawTiles(QPainter &p);
    void drawTrail(QPainter &p) const;
    void drawHome(QPainter &p) const;
    void drawVehicle(QPainter &p) const;
    void drawScaleBar(QPainter &p) const;
    void drawAttribution(QPainter &p) const;

    TileCache *m_cache;
    TileFetcher *m_fetcher;

    double m_centerLat = 39.925533; // Ankara, matching the SITL default home
    double m_centerLon = 32.866287;
    int m_zoom = 16;

    bool m_hasVehicle = false;
    double m_vehicleLat = 0;
    double m_vehicleLon = 0;
    float m_vehicleHeading = 0;
    float m_vehicleAltRel = 0;

    bool m_hasHome = false;
    double m_homeLat = 0;
    double m_homeLon = 0;

    QVector<QPointF> m_trail; // x = longitude, y = latitude
    bool m_followVehicle = true;
    bool m_dragging = false;
    QPoint m_lastDragPos;
};

} // namespace kerkenez
