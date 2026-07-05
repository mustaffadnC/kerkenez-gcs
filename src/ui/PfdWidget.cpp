#include "ui/PfdWidget.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>

#include <cmath>

namespace kerkenez {

namespace {

const QColor kSkyHigh(0x1f, 0x5a, 0x9e);
const QColor kSkyLow(0x6f, 0xa8, 0xdc);
const QColor kGroundHigh(0x8a, 0x5a, 0x2b);
const QColor kGroundLow(0x4e, 0x33, 0x18);
const QColor kTapeBackground(0, 0, 0, 110);
// Degrees of pitch across the widget height. Multicopters dash at 25°+ nose
// down; a wider window keeps the horizon on screen during aggressive moves.
constexpr double kVisiblePitchDeg = 70.0;

} // namespace

PfdWidget::PfdWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void PfdWidget::setAttitude(float rollDeg, float pitchDeg, float yawDeg)
{
    m_rollDeg = rollDeg;
    m_pitchDeg = pitchDeg;
    m_yawDeg = yawDeg;
    update();
}

void PfdWidget::setSpeeds(float airspeed, float groundspeed, float climbRate, int throttlePct)
{
    m_airspeed = airspeed;
    m_groundspeed = groundspeed;
    m_climbRate = climbRate;
    m_throttlePct = throttlePct;
    update();
}

void PfdWidget::setAltitudes(float altMsl, float altRel)
{
    m_altMsl = altMsl;
    m_altRel = altRel;
    update();
}

void PfdWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const QRectF r = rect();
    drawHorizonAndLadder(p, r);
    drawRollScale(p, r);
    drawAircraftSymbol(p, r);
    drawSpeedTape(p, r);
    drawAltitudeTape(p, r);

    p.setPen(QPen(QColor(30, 30, 30), 2));
    p.drawRect(r.adjusted(1, 1, -1, -1));
}

void PfdWidget::drawHorizonAndLadder(QPainter &p, const QRectF &r) const
{
    p.save();
    p.setClipRect(r);

    const double ppd = r.height() / kVisiblePitchDeg; // pixels per degree

    p.translate(r.center());
    p.rotate(-m_rollDeg);
    p.translate(0, m_pitchDeg * ppd);

    const double extent = r.width() + r.height();

    QLinearGradient sky(0, -extent, 0, 0);
    sky.setColorAt(0, kSkyHigh);
    sky.setColorAt(1, kSkyLow);
    p.fillRect(QRectF(-extent, -extent, 2 * extent, extent), sky);

    QLinearGradient ground(0, 0, 0, extent);
    ground.setColorAt(0, kGroundHigh);
    ground.setColorAt(1, kGroundLow);
    p.fillRect(QRectF(-extent, 0, 2 * extent, extent), ground);

    p.setPen(QPen(Qt::white, 2));
    p.drawLine(QPointF(-extent, 0), QPointF(extent, 0));

    // Pitch ladder: minor rung every 5°, major with label every 10°.
    QFont font = p.font();
    font.setPixelSize(11);
    p.setFont(font);
    const QFontMetricsF fm(font);
    for (int deg = -40; deg <= 40; deg += 5) {
        if (deg == 0)
            continue;
        const double y = -deg * ppd;
        const bool major = deg % 10 == 0;
        const double halfWidth = major ? 40.0 : 22.0;
        p.setPen(QPen(Qt::white, major ? 2.0 : 1.0));
        p.drawLine(QPointF(-halfWidth, y), QPointF(halfWidth, y));
        if (major) {
            const QString label = QString::number(qAbs(deg));
            p.drawText(QPointF(-halfWidth - fm.horizontalAdvance(label) - 6,
                               y + fm.ascent() / 2.0), label);
            p.drawText(QPointF(halfWidth + 6, y + fm.ascent() / 2.0), label);
        }
    }

    p.restore();
}

void PfdWidget::drawRollScale(QPainter &p, const QRectF &r) const
{
    const double radius = qMin(r.width(), r.height()) * 0.36;
    const QPointF center = r.center();

    p.save();
    p.setPen(QPen(Qt::white, 2));

    // Fixed tick marks; 0° at the top.
    for (const int angle : {-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60}) {
        p.save();
        p.translate(center);
        p.rotate(angle);
        const double tickLength = (angle == 0 || qAbs(angle) == 30 || qAbs(angle) == 60) ? 12 : 7;
        p.drawLine(QPointF(0, -radius), QPointF(0, -radius - tickLength));
        p.restore();
    }

    // Sky pointer: rotates with the horizon.
    p.translate(center);
    p.rotate(-m_rollDeg);
    QPainterPath pointer;
    pointer.moveTo(0, -radius + 2);
    pointer.lineTo(-7, -radius + 14);
    pointer.lineTo(7, -radius + 14);
    pointer.closeSubpath();
    p.fillPath(pointer, Qt::white);
    p.restore();
}

void PfdWidget::drawAircraftSymbol(QPainter &p, const QRectF &r) const
{
    const QPointF c = r.center();
    p.save();
    p.setPen(QPen(QColor(0xff, 0xd6, 0x00), 4, Qt::SolidLine, Qt::FlatCap));
    p.drawLine(c + QPointF(-56, 0), c + QPointF(-22, 0));
    p.drawLine(c + QPointF(22, 0), c + QPointF(56, 0));
    p.drawLine(c + QPointF(-22, 0), c + QPointF(-22, 10));
    p.drawLine(c + QPointF(22, 0), c + QPointF(22, 10));
    p.setBrush(QColor(0xff, 0xd6, 0x00));
    p.drawEllipse(c, 3, 3);
    p.restore();
}

namespace {

// Shared vertical tape: window centered on the current value.
void drawTape(QPainter &p, const QRectF &tape, double value, double unitsVisible,
              int minorStep, int labelStep, bool ticksOnRight, const QString &boxText,
              const QString &footer)
{
    p.save();
    p.fillRect(tape, kTapeBackground);
    p.setClipRect(tape);

    QFont font = p.font();
    font.setPixelSize(11);
    p.setFont(font);
    const QFontMetricsF fm(font);

    const double ppu = tape.height() / unitsVisible; // pixels per unit
    const double centerY = tape.center().y();
    const double tickX = ticksOnRight ? tape.right() : tape.left();
    const double tickDir = ticksOnRight ? -1.0 : 1.0;

    const int first = int(std::floor((value - unitsVisible / 2) / minorStep)) * minorStep;
    const int last = int(std::ceil((value + unitsVisible / 2) / minorStep)) * minorStep;
    p.setPen(QPen(Qt::white, 1));
    for (int v = first; v <= last; v += minorStep) {
        if (v < 0)
            continue;
        const double y = centerY - (v - value) * ppu;
        const bool labeled = v % labelStep == 0;
        p.drawLine(QPointF(tickX, y), QPointF(tickX + tickDir * (labeled ? 12 : 6), y));
        if (labeled) {
            const QString text = QString::number(v);
            const double textX = ticksOnRight
                ? tickX - 16 - fm.horizontalAdvance(text)
                : tickX + 16;
            p.drawText(QPointF(textX, y + fm.ascent() / 2.0), text);
        }
    }

    // Current-value box.
    QFont boxFont = p.font();
    boxFont.setPixelSize(15);
    boxFont.setBold(true);
    p.setFont(boxFont);
    const QFontMetricsF boxFm(boxFont);
    QRectF box(tape.left() + 3, centerY - 14, tape.width() - 6, 28);
    p.setClipping(false);
    p.fillRect(box, QColor(10, 10, 10, 230));
    p.setPen(QPen(Qt::white, 1.5));
    p.drawRect(box);
    p.drawText(box, Qt::AlignCenter, boxText);

    if (!footer.isEmpty()) {
        QFont footFont = p.font();
        footFont.setPixelSize(10);
        footFont.setBold(false);
        p.setFont(footFont);
        p.drawText(QRectF(tape.left(), tape.bottom() - 16, tape.width(), 14),
                   Qt::AlignCenter, footer);
    }
    p.restore();
}

} // namespace

void PfdWidget::drawSpeedTape(QPainter &p, const QRectF &r) const
{
    const QRectF tape(r.left() + 6, r.top() + r.height() * 0.12, 62, r.height() * 0.76);
    drawTape(p, tape, m_groundspeed, 20.0, 1, 5, /*ticksOnRight*/ true,
             QString::number(double(m_groundspeed), 'f', 1),
             QStringLiteral("GS m/s"));
}

void PfdWidget::drawAltitudeTape(QPainter &p, const QRectF &r) const
{
    const QRectF tape(r.right() - 68, r.top() + r.height() * 0.12, 62, r.height() * 0.76);
    drawTape(p, tape, m_altRel, 40.0, 1, 10, /*ticksOnRight*/ false,
             QString::number(double(m_altRel), 'f', 1),
             QStringLiteral("ALT m"));

    // Climb rate readout under the altitude tape.
    QFont font = p.font();
    font.setPixelSize(11);
    p.setFont(font);
    p.setPen(m_climbRate >= 0 ? QColor(0x9c, 0xff, 0x9c) : QColor(0xff, 0xb0, 0xb0));
    p.drawText(QRectF(tape.left(), tape.bottom() + 4, tape.width(), 16), Qt::AlignCenter,
               QStringLiteral("%1%2 m/s")
                   .arg(m_climbRate >= 0 ? QStringLiteral("+") : QString())
                   .arg(double(m_climbRate), 0, 'f', 1));
}

} // namespace kerkenez
