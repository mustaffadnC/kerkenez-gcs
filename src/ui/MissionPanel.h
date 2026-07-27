#pragma once

#include <QWidget>

#include "core/MissionItem.h"

class QDoubleSpinBox;
class QLabel;
class QTableWidget;

namespace kerkenez {

class MissionController;

// Editable waypoint list. The map edits the same plan through the slots below,
// so drawing and the table always show the same thing.
class MissionPanel : public QWidget
{
    Q_OBJECT
public:
    explicit MissionPanel(MissionController *controller, QWidget *parent = nullptr);

    MissionPlan plan() const { return m_plan; }
    float defaultAltitude() const;

public slots:
    void addWaypoint(double lat, double lon);
    void moveWaypoint(int index, double lat, double lon);
    void removeWaypoint(int index);
    void setPlan(const kerkenez::MissionPlan &plan);
    void uploadPlan();

signals:
    void planChanged(const kerkenez::MissionPlan &plan);

private:
    void appendCommand(uint16_t command);
    void refreshTable();
    void emitChanged();

    MissionController *m_controller;
    MissionPlan m_plan;
    QTableWidget *m_table = nullptr;
    QDoubleSpinBox *m_defaultAltitude = nullptr;
    QLabel *m_status = nullptr;
    bool m_updatingTable = false;
};

} // namespace kerkenez
