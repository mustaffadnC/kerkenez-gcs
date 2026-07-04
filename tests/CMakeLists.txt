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
