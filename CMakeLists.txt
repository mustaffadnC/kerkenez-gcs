cmake_minimum_required(VERSION 3.21)
project(kerkenez-gcs VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network SerialPort Widgets Test)

enable_testing()

# Vendored MAVLink c_library_v2 (header-only). SYSTEM silences warnings from
# generated packed-struct headers.
add_library(mavlink INTERFACE)
target_include_directories(mavlink SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/third_party/mavlink)

add_subdirectory(src/core)
add_subdirectory(src/comm)
add_subdirectory(src/ui)
add_subdirectory(src/app)
add_subdirectory(tests)
