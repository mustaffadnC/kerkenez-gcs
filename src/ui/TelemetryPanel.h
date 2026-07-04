#pragma once

#include <QWidget>

class QGridLayout;
class QLabel;
class QListWidget;

namespace kerkenez {

class Vehicle;

// Live raw-value grid + autopilot message log. The PFD replaces the grid as
// the primary display in Phase 2; this panel stays as a diagnostics view.
class TelemetryPanel : public QWidget
{
    Q_OBJECT
public:
    explicit TelemetryPanel(Vehicle *vehicle, QWidget *parent = nullptr);

private:
    QLabel *addValue(QGridLayout *grid, int row, int column, const QString &caption);

    QLabel *m_mode = nullptr;
    QLabel *m_armed = nullptr;
    QLabel *m_alive = nullptr;
    QLabel *m_roll = nullptr;
    QLabel *m_pitch = nullptr;
    QLabel *m_yaw = nullptr;
    QLabel *m_position = nullptr;
    QLabel *m_altitude = nullptr;
    QLabel *m_speed = nullptr;
    QLabel *m_climb = nullptr;
    QLabel *m_battery = nullptr;
    QLabel *m_gps = nullptr;
    QListWidget *m_messages = nullptr;
};

} // namespace kerkenez
