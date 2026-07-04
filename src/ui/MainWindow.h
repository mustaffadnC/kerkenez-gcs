#pragma once

#include <QMainWindow>

class QLabel;

namespace kerkenez {

class LinkManager;
class MavlinkCodec;
class Vehicle;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(LinkManager *linkManager, MavlinkCodec *codec, Vehicle *vehicle,
               QWidget *parent = nullptr);

private:
    void openConnectDialog();
    void updateStats();

    LinkManager *m_linkManager;
    MavlinkCodec *m_codec;

    QAction *m_connectAction = nullptr;
    QAction *m_disconnectAction = nullptr;
    QLabel *m_linkLabel = nullptr;
    QLabel *m_statsLabel = nullptr;
};

} // namespace kerkenez
