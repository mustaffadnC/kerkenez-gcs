#include "ui/StatusPanel.h"

#include <QGridLayout>
#include <QLabel>

#include "core/Vehicle.h"

namespace kerkenez {

StatusPanel::StatusPanel(Vehicle *vehicle, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QGridLayout(this);

    m_mode = new QLabel(QStringLiteral("—"), this);
    m_mode->setStyleSheet(QStringLiteral("font-size: 26px; font-weight: bold;"));
    m_mode->setAlignment(Qt::AlignCenter);

    m_armed = new QLabel(tr("DISARMED"), this);
    m_armed->setAlignment(Qt::AlignCenter);
    m_armed->setStyleSheet(QStringLiteral(
        "font-size: 17px; font-weight: bold; color: white; background: #455a64;"
        "border-radius: 4px; padding: 5px;"));

    m_battery = new QLabel(QStringLiteral("—"), this);
    m_gps = new QLabel(QStringLiteral("—"), this);
    for (QLabel *label : {m_battery, m_gps}) {
        label->setStyleSheet(QStringLiteral("font-size: 14px;"));
        label->setAlignment(Qt::AlignCenter);
    }

    layout->addWidget(m_mode, 0, 0, 1, 2);
    layout->addWidget(m_armed, 1, 0, 1, 2);
    layout->addWidget(m_battery, 2, 0);
    layout->addWidget(m_gps, 2, 1);

    connect(vehicle, &Vehicle::modeChanged, this, [this](const QString &mode, bool armed) {
        m_mode->setText(mode);
        m_armed->setText(armed ? tr("ARMED") : tr("DISARMED"));
        m_armed->setStyleSheet(QStringLiteral(
            "font-size: 17px; font-weight: bold; color: white; border-radius: 4px;"
            "padding: 5px; background: %1;").arg(armed ? QStringLiteral("#c62828")
                                                       : QStringLiteral("#455a64")));
    });
    connect(vehicle, &Vehicle::batteryChanged, this,
            [this](float voltage, float current, int remaining) {
                m_battery->setText(QStringLiteral("🔋 %1 V  %2 A  %3%")
                                       .arg(double(voltage), 0, 'f', 1)
                                       .arg(double(current), 0, 'f', 1)
                                       .arg(remaining));
            });
    connect(vehicle, &Vehicle::gpsChanged, this, [this](int fixType, int satellites) {
        static const char *fixNames[] = {"No GPS", "No Fix", "2D", "3D", "DGPS", "RTK-F", "RTK"};
        const QString fix = fixType >= 0 && fixType <= 6 ? QString::fromLatin1(fixNames[fixType])
                                                         : QString::number(fixType);
        m_gps->setText(QStringLiteral("🛰 %1 · %2 sat").arg(fix).arg(satellites));
    });
}

} // namespace kerkenez
