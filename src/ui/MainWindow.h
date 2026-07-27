#pragma once

#include <QMainWindow>

#include <memory>

class QLabel;

namespace kerkenez {

class LinkManager;
class MapWidget;
class MavlinkCodec;
class TileCache;
class TileFetcher;
class Vehicle;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(LinkManager *linkManager, MavlinkCodec *codec, Vehicle *vehicle,
               QWidget *parent = nullptr);
    ~MainWindow() override;

    void setMapOffline(bool offline);

private:
    void openConnectDialog();
    void updateStats();

    LinkManager *m_linkManager;
    MavlinkCodec *m_codec;

    std::unique_ptr<TileCache> m_tileCache;
    TileFetcher *m_tileFetcher = nullptr;
    MapWidget *m_map = nullptr;

    QAction *m_connectAction = nullptr;
    QAction *m_disconnectAction = nullptr;
    QAction *m_followAction = nullptr;
    QAction *m_offlineMapAction = nullptr;
    QLabel *m_linkLabel = nullptr;
    QLabel *m_statsLabel = nullptr;
};

} // namespace kerkenez
