#include "ui/MainWindow.h"

#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QSplitter>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include "comm/LinkManager.h"
#include "core/MavlinkCodec.h"
#include "core/Vehicle.h"
#include "ui/AlertPanel.h"
#include "ui/CompassWidget.h"
#include "ui/ConnectDialog.h"
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
    resize(1180, 720);

    // Left column: PFD with the compass tucked underneath.
    auto *pfd = new PfdWidget(this);
    auto *compass = new CompassWidget(this);
    compass->setFixedSize(170, 170);
    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(4, 4, 4, 4);
    leftLayout->addWidget(pfd, 1);
    auto *compassRow = new QHBoxLayout;
    compassRow->addStretch(1);
    compassRow->addWidget(compass);
    compassRow->addStretch(1);
    leftLayout->addLayout(compassRow);

    // Right column: status on top, alerts + autopilot log below.
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(4, 4, 4, 4);
    rightLayout->addWidget(new StatusPanel(vehicle, this));
    rightLayout->addWidget(new AlertPanel(vehicle, this), 1);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    setCentralWidget(splitter);

    // Raw values as an optional diagnostics dock.
    auto *dock = new QDockWidget(tr("Raw telemetry"), this);
    dock->setWidget(new TelemetryPanel(vehicle, dock));
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    dock->hide();
    menuBar()->addMenu(tr("&View"))->addAction(dock->toggleViewAction());

    connect(vehicle, &Vehicle::attitudeChanged, pfd, &PfdWidget::setAttitude);
    connect(vehicle, &Vehicle::vfrChanged, pfd, &PfdWidget::setSpeeds);
    connect(vehicle, &Vehicle::positionChanged, this,
            [pfd, compass](double, double, float msl, float rel, float heading) {
                pfd->setAltitudes(msl, rel);
                compass->setHeading(heading);
            });

    auto *toolbar = addToolBar(tr("Connection"));
    toolbar->setMovable(false);
    m_connectAction = toolbar->addAction(tr("Connect…"), this, &MainWindow::openConnectDialog);
    m_disconnectAction = toolbar->addAction(tr("Disconnect"), this, [this] {
        m_linkManager->disconnectLink();
    });
    m_disconnectAction->setEnabled(false);

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
