#pragma once

#include <QWidget>

class QLabel;

namespace kerkenez {

class Vehicle;

// Large-format vehicle status: flight mode, arming, battery, GPS.
class StatusPanel : public QWidget
{
    Q_OBJECT
public:
    explicit StatusPanel(Vehicle *vehicle, QWidget *parent = nullptr);

private:
    QLabel *m_mode = nullptr;
    QLabel *m_armed = nullptr;
    QLabel *m_battery = nullptr;
    QLabel *m_gps = nullptr;
};

} // namespace kerkenez
