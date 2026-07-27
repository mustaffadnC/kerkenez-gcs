add_library(kerkenez_map STATIC
    TileCache.h
    TileCache.cpp
    TileFetcher.h
    TileFetcher.cpp
    TileId.h
    TileMath.h
    TileMath.cpp
)

target_link_libraries(kerkenez_map
    PUBLIC Qt6::Core Qt6::Gui Qt6::Network
)
target_include_directories(kerkenez_map PUBLIC ${CMAKE_SOURCE_DIR}/src)
