#pragma once

#include <QMainWindow>

#include <memory>

#include "core/MavlinkDefs.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;

namespace kerkenez {

class CommandController;
class DemoMissionRunner;
class LinkManager;
class MapWidget;
class MavlinkCodec;
class MissionController;
class MissionPanel;
class ParamController;
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
    void startDemoMission();

private:
    void buildCommandToolbar(class QToolBar *toolbar);
    void openConnectDialog();
    void openParamDialog();
    void refreshModeList();
    void updateStats();

    LinkManager *m_linkManager;
    MavlinkCodec *m_codec;
    Vehicle *m_vehicle;

    CommandController *m_commands = nullptr;
    MissionController *m_missionController = nullptr;
    ParamController *m_params = nullptr;
    DemoMissionRunner *m_demo = nullptr;

    std::unique_ptr<TileCache> m_tileCache;
    TileFetcher *m_tileFetcher = nullptr;
    MapWidget *m_map = nullptr;
    MissionPanel *m_missionPanel = nullptr;

    QAction *m_connectAction = nullptr;
    QAction *m_disconnectAction = nullptr;
    QAction *m_followAction = nullptr;
    QAction *m_offlineMapAction = nullptr;
    QAction *m_armAction = nullptr;
    QComboBox *m_modeCombo = nullptr;
    QDoubleSpinBox *m_takeoffAltitude = nullptr;
    QLabel *m_linkLabel = nullptr;
    QLabel *m_statsLabel = nullptr;
    uint8_t m_modeListType = MAV_TYPE_GENERIC;
    bool m_updatingModeCombo = false;
};

} // namespace kerkenez
