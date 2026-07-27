#include "ui/MainWindow.h"

#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextStream>
#include <QTime>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include "comm/LinkManager.h"
#include "core/ApModes.h"
#include "core/CommandController.h"
#include "core/MavlinkCodec.h"
#include "core/MissionController.h"
#include "core/ParamController.h"
#include "core/Vehicle.h"
#include "map/TileCache.h"
#include "map/TileFetcher.h"
#include "ui/AlertPanel.h"
#include "ui/CompassWidget.h"
#include "ui/ConnectDialog.h"
#include "ui/DemoMissionRunner.h"
#include "ui/MapWidget.h"
#include "ui/MissionPanel.h"
#include "ui/ParamDialog.h"
#include "ui/PfdWidget.h"
#include "ui/StatusPanel.h"
#include "ui/TelemetryPanel.h"

namespace kerkenez {

MainWindow::MainWindow(LinkManager *linkManager, MavlinkCodec *codec, Vehicle *vehicle,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_linkManager(linkManager)
    , m_codec(codec)
    , m_vehicle(vehicle)
{
    setWindowTitle(QStringLiteral("Kerkenez GCS"));
    resize(1400, 820);

    m_commands = new CommandController(vehicle, this);
    m_missionController = new MissionController(vehicle, this);
    m_params = new ParamController(vehicle, this);
    connect(m_commands, &CommandController::sendMessage, m_linkManager, &LinkManager::send);
    connect(m_missionController, &MissionController::sendMessage, m_linkManager,
            &LinkManager::send);
    connect(m_params, &ParamController::sendMessage, m_linkManager, &LinkManager::send);
    connect(m_codec, &MavlinkCodec::messageReceived, m_commands,
            &CommandController::handleMessage);
    connect(m_codec, &MavlinkCodec::messageReceived, m_missionController,
            &MissionController::handleMessage);
    connect(m_codec, &MavlinkCodec::messageReceived, m_params, &ParamController::handleMessage);

    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + QStringLiteral("/tiles");
    m_tileCache = std::make_unique<TileCache>(cacheDir);
    m_tileFetcher = new TileFetcher(m_tileCache.get(), this);

    // Left column: instruments.
    auto *pfd = new PfdWidget(this);
    auto *compass = new CompassWidget(this);
    compass->setFixedSize(170, 170);
    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(4, 4, 4, 4);
    leftLayout->addWidget(pfd, 1);
    auto *compassRow = new QHBoxLayout;
    compassRow->addWidget(compass);
    compassRow->addWidget(new StatusPanel(vehicle, this), 1);
    leftLayout->addLayout(compassRow);

    // Right column: map over the message log / mission editor.
    m_map = new MapWidget(m_tileCache.get(), m_tileFetcher, this);
    m_missionPanel = new MissionPanel(m_missionController, this);
    auto *tabs = new QTabWidget(this);
    tabs->addTab(new AlertPanel(vehicle, this), tr("Messages"));
    tabs->addTab(m_missionPanel, tr("Mission"));

    auto *rightSplitter = new QSplitter(Qt::Vertical, this);
    rightSplitter->addWidget(m_map);
    rightSplitter->addWidget(tabs);
    rightSplitter->setStretchFactor(0, 4);
    rightSplitter->setStretchFactor(1, 1);
    // Stretch factors only govern later resizes; the initial split comes from
    // the size hints, and the panels' hints would crowd out the map.
    rightSplitter->setSizes({560, 240});

    auto *splitter = new QSplitter(this);
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightSplitter);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({480, 920});
    setCentralWidget(splitter);

    auto *dock = new QDockWidget(tr("Raw telemetry"), this);
    dock->setWidget(new TelemetryPanel(vehicle, dock));
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    dock->hide();

    connect(vehicle, &Vehicle::attitudeChanged, pfd, &PfdWidget::setAttitude);
    connect(vehicle, &Vehicle::vfrChanged, pfd, &PfdWidget::setSpeeds);
    connect(vehicle, &Vehicle::positionChanged, this,
            [this, pfd, compass](double lat, double lon, float msl, float rel, float heading) {
                pfd->setAltitudes(msl, rel);
                compass->setHeading(heading);
                m_map->setVehiclePosition(lat, lon, msl, rel, heading);
            });
    connect(vehicle, &Vehicle::homeChanged, m_map, &MapWidget::setHomePosition);
    connect(vehicle, &Vehicle::firstHeartbeat, m_map, &MapWidget::clearTrail);
    connect(vehicle, &Vehicle::firstHeartbeat, this, &MainWindow::refreshModeList);

    // Map editing drives the mission plan, and the plan drives what is drawn.
    connect(m_map, &MapWidget::waypointAdded, m_missionPanel, &MissionPanel::addWaypoint);
    connect(m_map, &MapWidget::waypointMoved, m_missionPanel, &MissionPanel::moveWaypoint);
    connect(m_map, &MapWidget::waypointRemoved, m_missionPanel, &MissionPanel::removeWaypoint);
    connect(m_missionPanel, &MissionPanel::planChanged, m_map, &MapWidget::setMissionPlan);
    connect(m_map, &MapWidget::flyToRequested, this, [this, tabs](double lat, double lon) {
        Q_UNUSED(tabs)
        const float altitude = m_vehicle->altitudeRelative() > 5.0f
            ? m_vehicle->altitudeRelative()
            : m_missionPanel->defaultAltitude();
        m_commands->flyTo(lat, lon, altitude);
        statusBar()->showMessage(tr("Guided target set at %1 m").arg(double(altitude)), 4000);
    });

    auto *toolbar = addToolBar(tr("Connection"));
    toolbar->setMovable(false);
    m_connectAction = toolbar->addAction(tr("Connect…"), this, &MainWindow::openConnectDialog);
    m_disconnectAction = toolbar->addAction(tr("Disconnect"), this, [this] {
        m_linkManager->disconnectLink();
    });
    m_disconnectAction->setEnabled(false);
    toolbar->addSeparator();

    m_followAction = toolbar->addAction(tr("Follow vehicle"));
    m_followAction->setCheckable(true);
    m_followAction->setChecked(m_map->followsVehicle());
    connect(m_followAction, &QAction::toggled, m_map, &MapWidget::setFollowVehicle);
    connect(m_map, &MapWidget::followVehicleChanged, m_followAction, &QAction::setChecked);
    toolbar->addAction(tr("Clear trail"), m_map, &MapWidget::clearTrail);
    toolbar->addSeparator();
    buildCommandToolbar(toolbar);

    auto *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(dock->toggleViewAction());
    viewMenu->addAction(m_followAction);
    m_offlineMapAction = viewMenu->addAction(tr("Offline map (cache only)"));
    m_offlineMapAction->setCheckable(true);
    connect(m_offlineMapAction, &QAction::toggled, this, &MainWindow::setMapOffline);

    m_linkLabel = new QLabel(tr("Disconnected"), this);
    m_statsLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_statsLabel);
    statusBar()->addWidget(m_linkLabel);

    connect(m_commands, &CommandController::commandFailed, this,
            [this](uint16_t command, const QString &reason) {
                statusBar()->showMessage(tr("Command %1 failed: %2").arg(command).arg(reason),
                                         6000);
            });

    connect(m_linkManager, &LinkManager::stateChanged, this, [this](ILink::State state) {
        const bool connected = state == ILink::State::Connected;
        m_connectAction->setEnabled(!connected);
        m_disconnectAction->setEnabled(m_linkManager->connectionWanted());
        switch (state) {
        case ILink::State::Connected:
            m_linkLabel->setText(tr("Connected — %1").arg(m_linkManager->linkDescription()));
            break;
        case ILink::State::Connecting:
            m_linkLabel->setText(tr("Connecting — %1").arg(m_linkManager->linkDescription()));
            break;
        case ILink::State::Disconnected:
            m_linkLabel->setText(m_linkManager->connectionWanted() ? tr("Link lost")
                                                                   : tr("Disconnected"));
            break;
        }
    });
    connect(m_linkManager, &LinkManager::reconnectScheduled, this, [this](int delayMs) {
        m_linkLabel->setText(tr("Link lost — reconnecting in %1 s…").arg(delayMs / 1000));
    });
    connect(m_linkManager, &LinkManager::errorOccurred, this, [this](const QString &message) {
        statusBar()->showMessage(tr("Link error: %1").arg(message), 5000);
    });

    auto *statsTimer = new QTimer(this);
    connect(statsTimer, &QTimer::timeout, this, &MainWindow::updateStats);
    statsTimer->start(1000);
}

MainWindow::~MainWindow() = default;

void MainWindow::buildCommandToolbar(QToolBar *toolbar)
{
    m_armAction = toolbar->addAction(tr("Arm"));
    connect(m_armAction, &QAction::triggered, this, [this] {
        const bool wantArm = !m_vehicle->armed();
        // Arming spins the propellers; make it a deliberate act.
        const auto answer = QMessageBox::question(
            this, wantArm ? tr("Arm vehicle") : tr("Disarm vehicle"),
            wantArm ? tr("Arm the vehicle in %1 mode?").arg(m_vehicle->modeName())
                    : tr("Disarm the vehicle?"));
        if (answer == QMessageBox::Yes)
            m_commands->arm(wantArm);
    });
    connect(m_vehicle, &Vehicle::modeChanged, this, [this](const QString &mode, bool armed) {
        m_armAction->setText(armed ? tr("Disarm") : tr("Arm"));
        if (!m_modeCombo || m_modeCombo->count() == 0
            || m_vehicle->vehicleType() != m_modeListType) {
            refreshModeList();
        }
        if (m_modeCombo) {
            m_updatingModeCombo = true;
            const int index = m_modeCombo->findText(mode);
            if (index >= 0)
                m_modeCombo->setCurrentIndex(index);
            m_updatingModeCombo = false;
        }
    });

    m_takeoffAltitude = new QDoubleSpinBox(this);
    m_takeoffAltitude->setRange(2, 500);
    m_takeoffAltitude->setValue(30);
    m_takeoffAltitude->setSuffix(tr(" m"));
    toolbar->addWidget(m_takeoffAltitude);
    toolbar->addAction(tr("Takeoff"), this, [this] {
        m_commands->takeoff(float(m_takeoffAltitude->value()));
    });
    toolbar->addAction(tr("RTL"), this, [this] { m_commands->returnToLaunch(); });
    toolbar->addAction(tr("Land"), this, [this] { m_commands->land(); });

    m_modeCombo = new QComboBox(this);
    m_modeCombo->setMinimumWidth(130);
    toolbar->addWidget(m_modeCombo);
    connect(m_modeCombo, &QComboBox::activated, this, [this](int index) {
        if (m_updatingModeCombo || index < 0)
            return;
        m_commands->setFlightMode(m_modeCombo->itemData(index).toUInt());
    });

    toolbar->addSeparator();
    toolbar->addAction(tr("Parameters…"), this, &MainWindow::openParamDialog);
}

void MainWindow::refreshModeList()
{
    if (!m_modeCombo)
        return;
    m_updatingModeCombo = true;
    m_modeCombo->clear();
    m_modeListType = m_vehicle->vehicleType();
    for (const auto &mode : apSelectableModes(m_modeListType))
        m_modeCombo->addItem(mode.first, mode.second);
    m_updatingModeCombo = false;
}

void MainWindow::startDemoMission()
{
    if (m_demo)
        return;
    m_demo = new DemoMissionRunner(m_vehicle, m_commands, m_missionController, this);
    connect(m_demo, &DemoMissionRunner::planReady, m_missionPanel, &MissionPanel::setPlan);
    connect(m_demo, &DemoMissionRunner::log, this, [this](const QString &line) {
        statusBar()->showMessage(tr("Demo: %1").arg(line), 8000);
        // A GUI build has no console, so the demo trace goes to a file.
        QFile log(QStringLiteral("demo-mission.log"));
        if (log.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream(&log) << QTime::currentTime().toString(Qt::ISODate) << ' ' << line
                              << '\n';
        }
    });
    m_demo->start();
}

void MainWindow::setMapOffline(bool offline)
{
    m_tileFetcher->setOffline(offline);
    if (m_offlineMapAction->isChecked() != offline)
        m_offlineMapAction->setChecked(offline);
    m_map->update();
}

void MainWindow::openConnectDialog()
{
    ConnectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
        m_linkManager->connectLink(dialog.config());
}

void MainWindow::openParamDialog()
{
    auto *dialog = new ParamDialog(m_params, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void MainWindow::updateStats()
{
    const quint64 received = m_codec->packetsReceived();
    const quint64 lost = m_codec->packetsLost();
    const double lossPct = (received + lost) > 0 ? 100.0 * lost / double(received + lost) : 0.0;
    m_statsLabel->setText(tr("pkts %1  loss %2%  crc %3")
                              .arg(received)
                              .arg(lossPct, 0, 'f', 1)
                              .arg(m_codec->crcErrors()));
}

} // namespace kerkenez
