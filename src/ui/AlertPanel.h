#pragma once

#include <QWidget>

class QLabel;
class QListWidget;

namespace kerkenez {

class Vehicle;

// Computed alert banners (link lost, battery, GPS) plus the autopilot
// STATUSTEXT log, colored by severity.
class AlertPanel : public QWidget
{
    Q_OBJECT
public:
    explicit AlertPanel(Vehicle *vehicle, QWidget *parent = nullptr);

private:
    void updateBanners();

    Vehicle *m_vehicle;
    QLabel *m_linkBanner = nullptr;
    QLabel *m_batteryBanner = nullptr;
    QLabel *m_gpsBanner = nullptr;
    QListWidget *m_log = nullptr;
    bool m_everAlive = false;
};

} // namespace kerkenez
