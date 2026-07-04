add_library(kerkenez_ui STATIC
    ConnectDialog.h
    ConnectDialog.cpp
    MainWindow.h
    MainWindow.cpp
    TelemetryPanel.h
    TelemetryPanel.cpp
)

target_link_libraries(kerkenez_ui
    PUBLIC kerkenez_core kerkenez_comm Qt6::Widgets
)
