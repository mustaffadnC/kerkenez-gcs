add_executable(tst_mavlinkcodec tst_mavlinkcodec.cpp)
target_link_libraries(tst_mavlinkcodec PRIVATE kerkenez_core Qt6::Test)
add_test(NAME mavlinkcodec COMMAND tst_mavlinkcodec)
