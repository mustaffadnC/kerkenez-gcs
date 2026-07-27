#include "ui/MainWindow.h"

#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include "comm/LinkManager.h"
#include "core/MavlinkCodec.h"
#include "core/Vehicle.h"
#include "map/TileCache.h"
#include "map/TileFetcher.h"
#include "ui/AlertPanel.h"
#include "ui/CompassWidget.h"
#include "ui/ConnectDialog.h"
#include "ui/MapWidget.h"
#include "ui/PfdWidget.h"
#include "ui/StatusPanel.h"
#include "ui/TelemetryPanel.h"

namespace kerkenez {

MainWindow::MainWindow(LinkManager *linkManager, MavlinkCodec *codec, Vehicle *vehicle,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_linkManager(linkManager)
    , m_codec(codec)
{
    setWindowTitle(QStringLiteral("Kerkenez GCS"));
    resize(1400, 820);

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

    // Right column: map over the alert log.
    m_map = new MapWidget(m_tileCache.get(), m_tileFetcher, this);
    auto *rightSplitter = new QSplitter(Qt::Vertical, this);
    rightSplitter->addWidget(m_map);
    rightSplitter->addWidget(new AlertPanel(vehicle, this));
    rightSplitter->setStretchFactor(0, 4);
    rightSplitter->setStretchFactor(1, 1);
    // Stretch factors only govern later resizes; the initial split comes from
    // the size hints, and the message log's hint would crowd out the map.
    rightSplitter->setSizes({620, 180});

    auto *splitter = new QSplitter(this);
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightSplitter);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({520, 880});
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
    // A new vehicle session starts a new track.
    connect(vehicle, &Vehicle::firstHeartbeat, m_map, &MapWidget::clearTrail);

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
