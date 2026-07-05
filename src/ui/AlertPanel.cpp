#include "ui/AlertPanel.h"

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

#include "core/Vehicle.h"

namespace kerkenez {

namespace {

constexpr int kMaxLogEntries = 200;

QLabel *makeBanner(QWidget *parent)
{
    auto *banner = new QLabel(parent);
    banner->setAlignment(Qt::AlignCenter);
    banner->setVisible(false);
    return banner;
}

void styleBanner(QLabel *banner, const QString &text, const QString &background)
{
    banner->setText(text);
    banner->setStyleSheet(QStringLiteral(
        "font-size: 14px; font-weight: bold; color: white; background: %1;"
        "border-radius: 4px; padding: 6px;").arg(background));
    banner->setVisible(true);
}

} // namespace

AlertPanel::AlertPanel(Vehicle *vehicle, QWidget *parent)
    : QWidget(parent)
    , m_vehicle(vehicle)
{
    auto *layout = new QVBoxLayout(this);
    m_linkBanner = makeBanner(this);
    m_batteryBanner = makeBanner(this);
    m_gpsBanner = makeBanner(this);
    m_log = new QListWidget(this);
    m_log->setSelectionMode(QAbstractItemView::NoSelection);

    layout->addWidget(m_linkBanner);
    layout->addWidget(m_batteryBanner);
    layout->addWidget(m_gpsBanner);
    layout->addWidget(m_log, 1);

    connect(vehicle, &Vehicle::aliveChanged, this, [this](bool alive) {
        if (alive)
            m_everAlive = true;
        updateBanners();
    });
    connect(vehicle, &Vehicle::batteryChanged, this, [this] { updateBanners(); });
    connect(vehicle, &Vehicle::gpsChanged, this, [this] { updateBanners(); });

    connect(vehicle, &Vehicle::statusTextReceived, this, [this](int severity, const QString &text) {
        auto *item = new QListWidgetItem(text);
        if (severity <= 3) // MAV_SEVERITY_ERROR and worse
            item->setForeground(QBrush(QColor(0xd3, 0x2f, 0x2f)));
        else if (severity == 4) // MAV_SEVERITY_WARNING
            item->setForeground(QBrush(QColor(0xf5, 0x7f, 0x17)));
        m_log->insertItem(0, item);
        while (m_log->count() > kMaxLogEntries)
            delete m_log->takeItem(m_log->count() - 1);
    });
}

void AlertPanel::updateBanners()
{
    if (!m_vehicle->isAlive() && m_everAlive)
        styleBanner(m_linkBanner, tr("TELEMETRY LOST"), QStringLiteral("#c62828"));
    else
        m_linkBanner->setVisible(false);

    const int remaining = m_vehicle->batteryRemainingPct();
    if (remaining >= 0 && remaining < 10)
        styleBanner(m_batteryBanner, tr("BATTERY CRITICAL — %1%").arg(remaining),
                    QStringLiteral("#c62828"));
    else if (remaining >= 0 && remaining < 20)
        styleBanner(m_batteryBanner, tr("BATTERY LOW — %1%").arg(remaining),
                    QStringLiteral("#ef6c00"));
    else
        m_batteryBanner->setVisible(false);

    if (m_vehicle->isAlive() && m_vehicle->gpsFixType() < 3)
        styleBanner(m_gpsBanner, tr("NO GPS FIX"), QStringLiteral("#ef6c00"));
    else
        m_gpsBanner->setVisible(false);
}

} // namespace kerkenez
