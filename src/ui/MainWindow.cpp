#include "ui/MainWindow.h"

#include <QLabel>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>

#include "comm/LinkManager.h"
#include "core/MavlinkCodec.h"
#include "core/Vehicle.h"
#include "ui/ConnectDialog.h"
#include "ui/TelemetryPanel.h"

namespace kerkenez {

MainWindow::MainWindow(LinkManager *linkManager, MavlinkCodec *codec, Vehicle *vehicle,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_linkManager(linkManager)
    , m_codec(codec)
{
    setWindowTitle(QStringLiteral("Kerkenez GCS"));
    resize(1024, 640);

    setCentralWidget(new TelemetryPanel(vehicle, this));

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
