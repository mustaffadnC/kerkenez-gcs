#include "ui/MissionPanel.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "core/MissionController.h"

namespace kerkenez {

MissionPanel::MissionPanel(MissionController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
{
    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels({tr("#"), tr("Command"), tr("Latitude"), tr("Longitude"),
                                        tr("Alt (m)")});
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    m_defaultAltitude = new QDoubleSpinBox(this);
    m_defaultAltitude->setRange(1, 500);
    m_defaultAltitude->setValue(40);
    m_defaultAltitude->setSuffix(tr(" m"));

    auto *addTakeoff = new QPushButton(tr("+ Takeoff"), this);
    auto *addRtl = new QPushButton(tr("+ RTL"), this);
    auto *addLand = new QPushButton(tr("+ Land"), this);
    auto *removeSelected = new QPushButton(tr("Delete"), this);
    auto *clear = new QPushButton(tr("Clear"), this);
    auto *upload = new QPushButton(tr("Upload"), this);
    auto *download = new QPushButton(tr("Download"), this);
    auto *clearVehicle = new QPushButton(tr("Erase on vehicle"), this);

    m_status = new QLabel(tr("Right-click the map to add waypoints."), this);
    m_status->setStyleSheet(QStringLiteral("color: gray;"));

    auto *editRow = new QHBoxLayout;
    editRow->addWidget(new QLabel(tr("Default alt"), this));
    editRow->addWidget(m_defaultAltitude);
    editRow->addWidget(addTakeoff);
    editRow->addWidget(addRtl);
    editRow->addWidget(addLand);
    editRow->addStretch(1);
    editRow->addWidget(removeSelected);
    editRow->addWidget(clear);

    auto *transferRow = new QHBoxLayout;
    transferRow->addWidget(upload);
    transferRow->addWidget(download);
    transferRow->addWidget(clearVehicle);
    transferRow->addStretch(1);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(editRow);
    layout->addWidget(m_table, 1);
    layout->addLayout(transferRow);
    layout->addWidget(m_status);

    connect(addTakeoff, &QPushButton::clicked, this, [this] {
        appendCommand(MAV_CMD_NAV_TAKEOFF);
    });
    connect(addRtl, &QPushButton::clicked, this, [this] {
        appendCommand(MAV_CMD_NAV_RETURN_TO_LAUNCH);
    });
    connect(addLand, &QPushButton::clicked, this, [this] { appendCommand(MAV_CMD_NAV_LAND); });
    connect(removeSelected, &QPushButton::clicked, this, [this] {
        const int row = m_table->currentRow();
        if (row >= 0)
            removeWaypoint(row);
    });
    connect(clear, &QPushButton::clicked, this, [this] { setPlan({}); });
    connect(upload, &QPushButton::clicked, this, &MissionPanel::uploadPlan);
    connect(download, &QPushButton::clicked, m_controller, &MissionController::download);
    connect(clearVehicle, &QPushButton::clicked, m_controller,
            &MissionController::clearOnVehicle);

    connect(m_controller, &MissionController::progress, this, [this](int done, int total) {
        m_status->setText(tr("Transferring %1/%2…").arg(done).arg(total));
    });
    connect(m_controller, &MissionController::uploadFinished, this, [this] {
        m_status->setText(tr("Mission uploaded (%1 items).").arg(m_plan.size()));
    });
    connect(m_controller, &MissionController::downloadFinished, this,
            [this](const MissionPlan &plan) {
                setPlan(plan);
                m_status->setText(tr("Downloaded %1 items from the vehicle.").arg(plan.size()));
            });
    connect(m_controller, &MissionController::cleared, this, [this] {
        m_status->setText(tr("Mission erased on the vehicle."));
    });
    connect(m_controller, &MissionController::failed, this, [this](const QString &reason) {
        m_status->setText(tr("Mission transfer failed: %1").arg(reason));
    });

    // Altitude is the only directly editable cell.
    connect(m_table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
        if (m_updatingTable || item->column() != 4)
            return;
        bool ok = false;
        const float value = item->text().toFloat(&ok);
        if (ok && item->row() < m_plan.size()) {
            m_plan[item->row()].altitude = value;
            emitChanged();
        } else {
            refreshTable();
        }
    });

    refreshTable();
}

float MissionPanel::defaultAltitude() const
{
    return float(m_defaultAltitude->value());
}

void MissionPanel::addWaypoint(double lat, double lon)
{
    MissionItem item;
    item.command = MAV_CMD_NAV_WAYPOINT;
    item.latitude = lat;
    item.longitude = lon;
    item.altitude = defaultAltitude();
    m_plan.append(item);
    refreshTable();
    emitChanged();
}

void MissionPanel::appendCommand(uint16_t command)
{
    MissionItem item;
    item.command = command;
    item.altitude = command == MAV_CMD_NAV_TAKEOFF ? defaultAltitude() : 0.0f;
    // Takeoff is drawn on the map, so give it the previous waypoint's position
    // when there is one; the autopilot climbs in place regardless.
    if (command == MAV_CMD_NAV_TAKEOFF && !m_plan.isEmpty()) {
        item.latitude = m_plan.last().latitude;
        item.longitude = m_plan.last().longitude;
    }
    m_plan.append(item);
    refreshTable();
    emitChanged();
}

void MissionPanel::moveWaypoint(int index, double lat, double lon)
{
    if (index < 0 || index >= m_plan.size())
        return;
    m_plan[index].latitude = lat;
    m_plan[index].longitude = lon;
    refreshTable();
    emitChanged();
}

void MissionPanel::removeWaypoint(int index)
{
    if (index < 0 || index >= m_plan.size())
        return;
    m_plan.removeAt(index);
    refreshTable();
    emitChanged();
}

void MissionPanel::setPlan(const MissionPlan &plan)
{
    m_plan = plan;
    refreshTable();
    emitChanged();
}

void MissionPanel::uploadPlan()
{
    if (m_plan.isEmpty()) {
        m_status->setText(tr("Nothing to upload — add waypoints first."));
        return;
    }
    m_controller->upload(m_plan);
}

void MissionPanel::refreshTable()
{
    m_updatingTable = true;
    m_table->setRowCount(m_plan.size());
    for (int row = 0; row < m_plan.size(); ++row) {
        const MissionItem &item = m_plan.at(row);
        const bool located = item.hasLocation() || item.command == MAV_CMD_NAV_TAKEOFF;

        auto setCell = [this, row](int column, const QString &text, bool editable) {
            auto *cell = m_table->item(row, column);
            if (!cell) {
                cell = new QTableWidgetItem;
                m_table->setItem(row, column, cell);
            }
            cell->setText(text);
            Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
            if (editable)
                flags |= Qt::ItemIsEditable;
            cell->setFlags(flags);
        };

        setCell(0, QString::number(row + 1), false);
        setCell(1, missionCommandName(item.command), false);
        setCell(2, located ? QString::number(item.latitude, 'f', 6) : QStringLiteral("—"), false);
        setCell(3, located ? QString::number(item.longitude, 'f', 6) : QStringLiteral("—"), false);
        setCell(4, QString::number(double(item.altitude), 'f', 1), true);
    }
    m_updatingTable = false;
}

void MissionPanel::emitChanged()
{
    emit planChanged(m_plan);
}

} // namespace kerkenez
