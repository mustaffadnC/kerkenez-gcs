#include "ui/CompassWidget.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>

#include <cmath>

namespace kerkenez {

CompassWidget::CompassWidget(QWidget *parent)
    : QWidget(parent)
{
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
}

void CompassWidget::setHeading(float headingDeg)
{
    m_headingDeg = headingDeg;
    update();
}

void CompassWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const double side = qMin(width(), height());
    const QPointF center(width() / 2.0, height() / 2.0);
    const double radius = side / 2.0 - 10;

    p.setBrush(QColor(20, 20, 20));
    p.setPen(QPen(QColor(70, 70, 70), 2));
    p.drawEllipse(center, radius, radius);

    // Rose rotates so the current heading sits under the fixed top index.
    p.save();
    p.translate(center);
    p.rotate(-m_headingDeg);

    QFont font = p.font();
    font.setPixelSize(int(radius * 0.16));
    font.setBold(true);
    p.setFont(font);
    const QFontMetricsF fm(font);

    for (int angle = 0; angle < 360; angle += 10) {
        p.save();
        p.rotate(angle);
        const bool major = angle % 30 == 0;
        p.setPen(QPen(Qt::white, major ? 2.0 : 1.0));
        p.drawLine(QPointF(0, -radius + 2), QPointF(0, -radius + (major ? 12 : 7)));
        if (major) {
            QString label;
            switch (angle) {
            case 0: label = QStringLiteral("N"); break;
            case 90: label = QStringLiteral("E"); break;
            case 180: label = QStringLiteral("S"); break;
            case 270: label = QStringLiteral("W"); break;
            default: label = QString::number(angle / 10); break;
            }
            p.setPen(angle == 0 ? QColor(0xff, 0x66, 0x44) : Qt::white);
            p.drawText(QPointF(-fm.horizontalAdvance(label) / 2.0, -radius + 14 + fm.ascent()),
                       label);
        }
        p.restore();
    }
    p.restore();

    // Fixed index triangle at the top.
    QPainterPath index;
    index.moveTo(center.x(), center.y() - radius + 1);
    index.lineTo(center.x() - 7, center.y() - radius - 9);
    index.lineTo(center.x() + 7, center.y() - radius - 9);
    index.closeSubpath();
    p.fillPath(index, QColor(0xff, 0xd6, 0x00));

    // Digital readout.
    QFont readout = p.font();
    readout.setPixelSize(int(radius * 0.28));
    readout.setBold(true);
    p.setFont(readout);
    p.setPen(Qt::white);
    const int heading = (int(std::lround(m_headingDeg)) % 360 + 360) % 360;
    p.drawText(QRectF(center.x() - radius, center.y() - radius * 0.35, 2 * radius, radius * 0.7),
               Qt::AlignCenter, QStringLiteral("%1°").arg(heading, 3, 10, QLatin1Char('0')));
}

} // namespace kerkenez
