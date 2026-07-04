add_library(kerkenez_ui STATIC
    MainWindow.h
    MainWindow.cpp
)

target_link_libraries(kerkenez_ui
    PUBLIC kerkenez_core Qt6::Widgets
)
