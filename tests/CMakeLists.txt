add_executable(tst_mavlinkcodec tst_mavlinkcodec.cpp)
target_link_libraries(tst_mavlinkcodec PRIVATE kerkenez_core Qt6::Test)
add_test(NAME mavlinkcodec COMMAND tst_mavlinkcodec)

add_executable(tst_streamfixture tst_streamfixture.cpp)
target_link_libraries(tst_streamfixture PRIVATE kerkenez_core Qt6::Test)
target_compile_definitions(tst_streamfixture PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
add_test(NAME streamfixture COMMAND tst_streamfixture)

add_executable(tst_vehicle tst_vehicle.cpp)
target_link_libraries(tst_vehicle PRIVATE kerkenez_core Qt6::Test)
add_test(NAME vehicle COMMAND tst_vehicle)

add_executable(tst_linkmanager tst_linkmanager.cpp)
target_link_libraries(tst_linkmanager PRIVATE kerkenez_comm Qt6::Test)
add_test(NAME linkmanager COMMAND tst_linkmanager)

add_executable(tst_commandcontroller tst_commandcontroller.cpp MavlinkTestUtils.h)
target_link_libraries(tst_commandcontroller PRIVATE kerkenez_core Qt6::Test)
target_include_directories(tst_commandcontroller PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME commandcontroller COMMAND tst_commandcontroller)

add_executable(tst_missioncontroller tst_missioncontroller.cpp MavlinkTestUtils.h)
target_link_libraries(tst_missioncontroller PRIVATE kerkenez_core Qt6::Test)
target_include_directories(tst_missioncontroller PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME missioncontroller COMMAND tst_missioncontroller)

add_executable(tst_paramcontroller tst_paramcontroller.cpp MavlinkTestUtils.h)
target_link_libraries(tst_paramcontroller PRIVATE kerkenez_core Qt6::Test)
target_include_directories(tst_paramcontroller PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME paramcontroller COMMAND tst_paramcontroller)

add_executable(tst_tilemath tst_tilemath.cpp)
target_link_libraries(tst_tilemath PRIVATE kerkenez_map Qt6::Test)
add_test(NAME tilemath COMMAND tst_tilemath)

add_executable(tst_tilecache tst_tilecache.cpp)
target_link_libraries(tst_tilecache PRIVATE kerkenez_map Qt6::Test)
add_test(NAME tilecache COMMAND tst_tilecache)

add_executable(tst_widgets tst_widgets.cpp)
target_link_libraries(tst_widgets PRIVATE kerkenez_ui Qt6::Test)
add_test(NAME widgets COMMAND tst_widgets)
set_tests_properties(widgets PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
