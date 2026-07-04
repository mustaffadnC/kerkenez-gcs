# Main GUI application
add_executable(kerkenez WIN32 main.cpp)
target_link_libraries(kerkenez PRIVATE kerkenez_ui kerkenez_comm)

# Phase 0 console proof-of-concept: live telemetry from SITL
add_executable(poc_telemetry poc_telemetry.cpp)
target_link_libraries(poc_telemetry PRIVATE kerkenez_comm)
