#include "ui/TelemetryPanel.h"

#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "core/Vehicle.h"

namespace kerkenez {

namespace {
const auto kValueStyle = QStringLiteral("font-family: Consolas, monospace; font-size: 14px;");
} // namespace

TelemetryPanel::TelemetryPanel(Vehicle *vehicle, QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout;

    m_mode = addValue(grid, 0, 0, tr("Mode"));
    m_armed = addValue(grid, 0, 1, tr("Armed"));
    m_alive = addValue(grid, 0, 2, tr("Telemetry"));
    m_roll = addValue(grid, 1, 0, tr("Roll"));
    m_pitch = addValue(grid, 1, 1, tr("Pitch"));
    m_yaw = addValue(grid, 1, 2, tr("Yaw"));
    m_position = addValue(grid, 2, 0, tr("Position"));
    m_altitude = addValue(grid, 2, 1, tr("Altitude MSL / Rel"));
    m_gps = addValue(grid, 2, 2, tr("GPS"));
    m_speed = addValue(grid, 3, 0, tr("Ground / Air speed"));
    m_climb = addValue(grid, 3, 1, tr("Climb / Throttle"));
    m_battery = addValue(grid, 3, 2, tr("Battery"));

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(grid);
    layout->addStretch(1);

    connect(vehicle, &Vehicle::modeChanged, this, [this](const QString &mode, bool armed) {
        m_mode->setText(mode);
        m_armed->setText(armed ? tr("ARMED") : tr("Disarmed"));
    });
    connect(vehicle, &Vehicle::aliveChanged, this, [this](bool alive) {
        m_alive->setText(alive ? tr("LIVE") : tr("LOST"));
    });
    connect(vehicle, &Vehicle::attitudeChanged, this,
            [this](float roll, float pitch, float yaw) {
                m_roll->setText(QStringLiteral("%1°").arg(double(roll), 0, 'f', 1));
                m_pitch->setText(QStringLiteral("%1°").arg(double(pitch), 0, 'f', 1));
                m_yaw->setText(QStringLiteral("%1°").arg(double(yaw), 0, 'f', 1));
            });
    connect(vehicle, &Vehicle::positionChanged, this,
            [this](double lat, double lon, float msl, float rel, float) {
                m_position->setText(QStringLiteral("%1, %2")
                                        .arg(lat, 0, 'f', 6)
                                        .arg(lon, 0, 'f', 6));
                m_altitude->setText(QStringLiteral("%1 m / %2 m")
                                        .arg(double(msl), 0, 'f', 1)
                                        .arg(double(rel), 0, 'f', 1));
            });
    connect(vehicle, &Vehicle::vfrChanged, this,
            [this](float airspeed, float groundspeed, float climb, int throttle) {
                m_speed->setText(QStringLiteral("%1 / %2 m/s")
                                     .arg(double(groundspeed), 0, 'f', 1)
                                     .arg(double(airspeed), 0, 'f', 1));
                m_climb->setText(QStringLiteral("%1 m/s / %2%")
                                     .arg(double(climb), 0, 'f', 1)
                                     .arg(throttle));
            });
    connect(vehicle, &Vehicle::batteryChanged, this,
            [this](float voltage, float current, int remaining) {
                m_battery->setText(QStringLiteral("%1 V  %2 A  %3%")
                                       .arg(double(voltage), 0, 'f', 1)
                                       .arg(double(current), 0, 'f', 1)
                                       .arg(remaining));
            });
    connect(vehicle, &Vehicle::gpsChanged, this, [this](int fixType, int satellites) {
        m_gps->setText(QStringLiteral("fix %1, %2 sats").arg(fixType).arg(satellites));
    });
}

QLabel *TelemetryPanel::addValue(QGridLayout *grid, int row, int column, const QString &caption)
{
    auto *box = new QVBoxLayout;
    auto *captionLabel = new QLabel(caption, this);
    captionLabel->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    auto *valueLabel = new QLabel(QStringLiteral("—"), this);
    valueLabel->setStyleSheet(kValueStyle);
    box->addWidget(captionLabel);
    box->addWidget(valueLabel);
    grid->addLayout(box, row, column);
    return valueLabel;
}

} // namespace kerkenez
