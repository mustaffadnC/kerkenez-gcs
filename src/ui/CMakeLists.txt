add_library(kerkenez_ui STATIC
    AlertPanel.h
    AlertPanel.cpp
    CompassWidget.h
    CompassWidget.cpp
    ConnectDialog.h
    ConnectDialog.cpp
    MainWindow.h
    MainWindow.cpp
    PfdWidget.h
    PfdWidget.cpp
    StatusPanel.h
    StatusPanel.cpp
    TelemetryPanel.h
    TelemetryPanel.cpp
)

target_link_libraries(kerkenez_ui
    PUBLIC kerkenez_core kerkenez_comm Qt6::Widgets
)
