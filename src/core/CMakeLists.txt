add_library(kerkenez_core STATIC
    ApModes.h
    ApModes.cpp
    CommandController.h
    CommandController.cpp
    GcsMessages.h
    GcsMessages.cpp
    MavlinkDefs.h
    MavlinkCodec.h
    MavlinkCodec.cpp
    MissionController.h
    MissionController.cpp
    MissionItem.h
    MissionItem.cpp
    ParamController.h
    ParamController.cpp
    Vehicle.h
    Vehicle.cpp
)

target_link_libraries(kerkenez_core PUBLIC Qt6::Core mavlink)
target_include_directories(kerkenez_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
