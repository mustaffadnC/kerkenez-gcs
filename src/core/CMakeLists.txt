add_library(kerkenez_core STATIC
    MavlinkDefs.h
    MavlinkCodec.h
    MavlinkCodec.cpp
)

target_link_libraries(kerkenez_core PUBLIC Qt6::Core mavlink)
target_include_directories(kerkenez_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
