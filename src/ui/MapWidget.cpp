#include "ui/MapWidget.h"

#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <cmath>

#include "map/TileCache.h"
#include "map/TileFetcher.h"
#include "map/TileMath.h"

namespace kerkenez {

namespace {

constexpr int kMaxTrailPoints = 4000;
constexpr double kTrailMinMeters = 1.5; // thin out samples while hovering
const QColor kMissingTile(0x2b, 0x2f, 0x33);
const QColor kTrailColor(0xff, 0xd6, 0x00);
const QColor kMissionColor(0x21, 0x96, 0xf3);
constexpr double kWaypointRadius = 11.0;

} // namespace

MapWidget::MapWidget(TileCache *cache, TileFetcher *fetcher, QWidget *parent)
    : QWidget(parent)
    , m_cache(cache)
    , m_fetcher(fetcher)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(false);
    setCursor(Qt::OpenHandCursor);
    connect(m_fetcher, &TileFetcher::tileReady, this, [this](const TileId &) { update(); });
}

void MapWidget::setCenter(double lat, double lon)
{
    m_centerLat = lat;
    m_centerLon = lon;
    update();
}

void MapWidget::setZoom(int zoom)
{
    const int clamped = qBound(TileMath::kMinZoom, zoom, TileMath::kMaxZoom);
    if (clamped == m_zoom)
        return;
    m_zoom = clamped;
    update();
}

void MapWidget::setVehiclePosition(double lat, double lon, float, float altRel, float headingDeg)
{
    // The autopilot streams a position before the EKF has an origin; (0,0) is
    // in the Atlantic and would yank the map away from the real home.
    if (qFuzzyIsNull(lat) && qFuzzyIsNull(lon))
        return;

    m_hasVehicle = true;
    m_vehicleLat = lat;
    m_vehicleLon = lon;
    m_vehicleHeading = headingDeg;
    m_vehicleAltRel = altRel;

    if (m_trail.isEmpty()
        || TileMath::haversineMeters(m_trail.last().y(), m_trail.last().x(), lat, lon)
            > kTrailMinMeters) {
        m_trail.append(QPointF(lon, lat));
        if (m_trail.size() > kMaxTrailPoints)
            m_trail.remove(0, m_trail.size() - kMaxTrailPoints);
    }

    if (m_followVehicle) {
        m_centerLat = lat;
        m_centerLon = lon;
    }
    update();
}

void MapWidget::setHomePosition(double lat, double lon)
{
    m_hasHome = true;
    m_homeLat = lat;
    m_homeLon = lon;
    update();
}

void MapWidget::setFollowVehicle(bool follow)
{
    if (m_followVehicle == follow)
        return;
    m_followVehicle = follow;
    if (follow && m_hasVehicle)
        setCenter(m_vehicleLat, m_vehicleLon);
    emit followVehicleChanged(follow);
    update();
}

void MapWidget::setMissionPlan(const MissionPlan &plan)
{
    m_mission = plan;
    update();
}

void MapWidget::clearTrail()
{
    m_trail.clear();
    update();
}

void MapWidget::centerOnVehicle()
{
    if (m_hasVehicle)
        setCenter(m_vehicleLat, m_vehicleLon);
}

QPointF MapWidget::geoToScreen(double lat, double lon) const
{
    const double centerX = TileMath::lonToTileX(m_centerLon, m_zoom);
    const double centerY = TileMath::latToTileY(m_centerLat, m_zoom);
    const double x = TileMath::lonToTileX(lon, m_zoom);
    const double y = TileMath::latToTileY(lat, m_zoom);
    return QPointF(width() / 2.0 + (x - centerX) * TileMath::kTileSize,
                   height() / 2.0 + (y - centerY) * TileMath::kTileSize);
}

void MapWidget::screenToGeo(const QPointF &point, double *lat, double *lon) const
{
    const double centerX = TileMath::lonToTileX(m_centerLon, m_zoom);
    const double centerY = TileMath::latToTileY(m_centerLat, m_zoom);
    const double x = centerX + (point.x() - width() / 2.0) / TileMath::kTileSize;
    const double y = centerY + (point.y() - height() / 2.0) / TileMath::kTileSize;
    if (lon)
        *lon = TileMath::tileXToLon(x, m_zoom);
    if (lat)
        *lat = TileMath::tileYToLat(y, m_zoom);
}

void MapWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    drawTiles(p);
    drawTrail(p);
    drawMission(p);
    drawHome(p);
    drawVehicle(p);
    drawScaleBar(p);
    drawAttribution(p);

    p.setPen(QPen(QColor(30, 30, 30), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect().adjusted(1, 1, -1, -1));
}

void MapWidget::drawTiles(QPainter &p)
{
    p.fillRect(rect(), kMissingTile);

    const double centerX = TileMath::lonToTileX(m_centerLon, m_zoom);
    const double centerY = TileMath::latToTileY(m_centerLat, m_zoom);
    const double originX = width() / 2.0 - centerX * TileMath::kTileSize;
    const double originY = height() / 2.0 - centerY * TileMath::kTileSize;

    const int firstX = int(std::floor(-originX / TileMath::kTileSize));
    const int lastX = int(std::floor((width() - originX) / TileMath::kTileSize));
    const int firstY = int(std::floor(-originY / TileMath::kTileSize));
    const int lastY = int(std::floor((height() - originY) / TileMath::kTileSize));
    const int count = TileMath::tileCount(m_zoom);

    for (int ty = firstY; ty <= lastY; ++ty) {
        if (ty < 0 || ty >= count)
            continue;
        for (int tx = firstX; tx <= lastX; ++tx) {
            const TileId id{m_zoom, TileMath::wrapTileX(tx, m_zoom), ty};
            const QRectF target(originX + tx * TileMath::kTileSize,
                                originY + ty * TileMath::kTileSize,
                                TileMath::kTileSize, TileMath::kTileSize);
            QImage image;
            if (m_cache->get(id, &image)) {
                p.drawImage(target, image);
            } else {
                m_fetcher->request(id);
                p.setPen(QPen(QColor(0x3a, 0x3f, 0x45), 1));
                p.drawRect(target);
            }
        }
    }
}

void MapWidget::drawTrail(QPainter &p) const
{
    if (m_trail.size() < 2)
        return;
    QPainterPath path;
    path.moveTo(geoToScreen(m_trail.first().y(), m_trail.first().x()));
    for (int i = 1; i < m_trail.size(); ++i)
        path.lineTo(geoToScreen(m_trail.at(i).y(), m_trail.at(i).x()));

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(0, 0, 0, 120), 5));
    p.drawPath(path);
    p.setPen(QPen(kTrailColor, 2.5));
    p.drawPath(path);
}

void MapWidget::drawMission(QPainter &p) const
{
    if (m_mission.isEmpty())
        return;

    p.save();

    QPainterPath route;
    bool started = false;
    for (const MissionItem &item : m_mission) {
        if (!item.hasLocation() && item.command != MAV_CMD_NAV_TAKEOFF)
            continue;
        const QPointF pos = geoToScreen(item.latitude, item.longitude);
        if (!started) {
            route.moveTo(pos);
            started = true;
        } else {
            route.lineTo(pos);
        }
    }
    if (started) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(0, 0, 0, 120), 5));
        p.drawPath(route);
        p.setPen(QPen(kMissionColor, 2.5, Qt::DashLine));
        p.drawPath(route);
    }

    QFont font = p.font();
    font.setPixelSize(11);
    font.setBold(true);
    p.setFont(font);
    for (int i = 0; i < m_mission.size(); ++i) {
        const MissionItem &item = m_mission.at(i);
        if (!item.hasLocation() && item.command != MAV_CMD_NAV_TAKEOFF)
            continue;
        const QPointF pos = geoToScreen(item.latitude, item.longitude);
        p.setPen(QPen(Qt::black, 2));
        p.setBrush(item.command == MAV_CMD_NAV_TAKEOFF ? QColor(0x8b, 0xc3, 0x4a)
                                                       : kMissionColor);
        p.drawEllipse(pos, kWaypointRadius, kWaypointRadius);
        p.setPen(Qt::white);
        p.drawText(QRectF(pos.x() - kWaypointRadius, pos.y() - kWaypointRadius,
                          2 * kWaypointRadius, 2 * kWaypointRadius),
                   Qt::AlignCenter, QString::number(i + 1));
    }
    p.restore();
}

int MapWidget::waypointAt(const QPointF &point) const
{
    for (int i = m_mission.size() - 1; i >= 0; --i) {
        const MissionItem &item = m_mission.at(i);
        if (!item.hasLocation() && item.command != MAV_CMD_NAV_TAKEOFF)
            continue;
        const QPointF pos = geoToScreen(item.latitude, item.longitude);
        if (QLineF(pos, point).length() <= kWaypointRadius + 2)
            return i;
    }
    return -1;
}

void MapWidget::showContextMenu(const QPoint &position)
{
    double lat = 0, lon = 0;
    screenToGeo(position, &lat, &lon);
    const int hit = waypointAt(position);

    QMenu menu(this);
    QAction *flyHere = menu.addAction(tr("Fly here (Guided)"));
    QAction *addWaypoint = menu.addAction(tr("Add waypoint here"));
    QAction *remove = hit >= 0 ? menu.addAction(tr("Delete waypoint %1").arg(hit + 1)) : nullptr;

    QAction *chosen = menu.exec(mapToGlobal(position));
    if (!chosen)
        return;
    if (chosen == flyHere)
        emit flyToRequested(lat, lon);
    else if (chosen == addWaypoint)
        emit waypointAdded(lat, lon);
    else if (chosen == remove)
        emit waypointRemoved(hit);
}

void MapWidget::drawHome(QPainter &p) const
{
    if (!m_hasHome)
        return;
    const QPointF pos = geoToScreen(m_homeLat, m_homeLon);
    p.save();
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(QColor(0x43, 0xa0, 0x47));
    QPainterPath marker;
    marker.moveTo(pos + QPointF(0, 2));
    marker.lineTo(pos + QPointF(-9, -16));
    marker.lineTo(pos + QPointF(9, -16));
    marker.closeSubpath();
    p.drawPath(marker);
    p.setPen(QPen(Qt::white, 1));
    p.drawText(pos + QPointF(12, -8), QStringLiteral("HOME"));
    p.restore();
}

void MapWidget::drawVehicle(QPainter &p) const
{
    if (!m_hasVehicle)
        return;
    const QPointF pos = geoToScreen(m_vehicleLat, m_vehicleLon);

    p.save();
    p.translate(pos);
    p.rotate(m_vehicleHeading);
    QPainterPath arrow;
    arrow.moveTo(0, -16);
    arrow.lineTo(11, 12);
    arrow.lineTo(0, 6);
    arrow.lineTo(-11, 12);
    arrow.closeSubpath();
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(QColor(0xff, 0x52, 0x52));
    p.drawPath(arrow);
    p.restore();

    const QString label = QStringLiteral("%1 m").arg(double(m_vehicleAltRel), 0, 'f', 1);
    const QRectF box(pos.x() + 14, pos.y() - 10, 66, 20);
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 150));
    p.drawRoundedRect(box, 3, 3);
    p.setPen(Qt::white);
    p.drawText(box, Qt::AlignCenter, label);
    p.restore();
}

void MapWidget::drawScaleBar(QPainter &p) const
{
    const double mpp = TileMath::metersPerPixel(m_centerLat, m_zoom);
    if (mpp <= 0)
        return;

    // Pick a round distance that fits in ~120 px.
    static const double steps[] = {10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000};
    double meters = steps[0];
    for (const double candidate : steps) {
        if (candidate / mpp <= 120)
            meters = candidate;
    }
    const double pixels = meters / mpp;
    const double y = height() - 18;
    const double x = 12;

    p.save();
    p.setPen(QPen(Qt::black, 3));
    p.drawLine(QPointF(x, y), QPointF(x + pixels, y));
    p.setPen(QPen(Qt::white, 1.5));
    p.drawLine(QPointF(x, y), QPointF(x + pixels, y));
    p.drawLine(QPointF(x, y - 4), QPointF(x, y + 4));
    p.drawLine(QPointF(x + pixels, y - 4), QPointF(x + pixels, y + 4));

    const QString label = meters >= 1000 ? QStringLiteral("%1 km").arg(meters / 1000)
                                         : QStringLiteral("%1 m").arg(meters);
    p.setPen(Qt::black);
    p.drawText(QPointF(x + pixels + 7, y + 4), label);
    p.setPen(Qt::white);
    p.drawText(QPointF(x + pixels + 6, y + 3), label);
    p.restore();
}

void MapWidget::drawAttribution(QPainter &p) const
{
    // Required by the OSM tile usage policy.
    const QString text = QStringLiteral("© OpenStreetMap contributors");
    p.save();
    QFont font = p.font();
    font.setPixelSize(10);
    p.setFont(font);
    const QRectF box(width() - 176.0, height() - 16.0, 172.0, 14.0);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 170));
    p.drawRect(box);
    p.setPen(QColor(40, 40, 40));
    p.drawText(box, Qt::AlignCenter, text);
    p.restore();
}

void MapWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        showContextMenu(event->pos());
        return;
    }
    if (event->button() != Qt::LeftButton)
        return;

    m_draggedWaypoint = waypointAt(event->position());
    if (m_draggedWaypoint >= 0) {
        setCursor(Qt::SizeAllCursor);
        return;
    }
    m_dragging = true;
    m_lastDragPos = event->pos();
    setCursor(Qt::ClosedHandCursor);
}

void MapWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_draggedWaypoint >= 0) {
        double lat = 0, lon = 0;
        screenToGeo(event->position(), &lat, &lon);
        emit waypointMoved(m_draggedWaypoint, lat, lon);
        return;
    }
    if (!m_dragging)
        return;

    const QPoint delta = event->pos() - m_lastDragPos;
    m_lastDragPos = event->pos();

    const double centerX = TileMath::lonToTileX(m_centerLon, m_zoom)
        - double(delta.x()) / TileMath::kTileSize;
    const double centerY = TileMath::latToTileY(m_centerLat, m_zoom)
        - double(delta.y()) / TileMath::kTileSize;
    m_centerLon = TileMath::tileXToLon(centerX, m_zoom);
    m_centerLat = TileMath::tileYToLat(centerY, m_zoom);

    // Panning is an explicit "look elsewhere", so it releases the follow lock.
    if (m_followVehicle)
        setFollowVehicle(false);
    update();
}

void MapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    m_dragging = false;
    m_draggedWaypoint = -1;
    setCursor(Qt::OpenHandCursor);
}

void MapWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    double lat = 0, lon = 0;
    screenToGeo(event->position(), &lat, &lon);
    setFollowVehicle(false);
    setCenter(lat, lon);
    setZoom(m_zoom + 1);
}

void MapWidget::wheelEvent(QWheelEvent *event)
{
    const int steps = event->angleDelta().y() / 120;
    if (steps == 0)
        return;

    // Keep the geographic point under the cursor pinned while zooming.
    double anchorLat = 0, anchorLon = 0;
    screenToGeo(event->position(), &anchorLat, &anchorLon);
    const int before = m_zoom;
    setZoom(m_zoom + steps);
    if (m_zoom == before)
        return;

    if (!m_followVehicle) {
        const QPointF anchorAfter = geoToScreen(anchorLat, anchorLon);
        const QPointF drift = anchorAfter - event->position();
        const double centerX = TileMath::lonToTileX(m_centerLon, m_zoom)
            + drift.x() / TileMath::kTileSize;
        const double centerY = TileMath::latToTileY(m_centerLat, m_zoom)
            + drift.y() / TileMath::kTileSize;
        m_centerLon = TileMath::tileXToLon(centerX, m_zoom);
        m_centerLat = TileMath::tileYToLat(centerY, m_zoom);
    }
    update();
}

} // namespace kerkenez
