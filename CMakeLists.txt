cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
cmake_policy(SET CMP0091 NEW)

project(NeuralAmpModeler VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${ipo_supported})

set(PLUG_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/plugin/resources)
include(${CMAKE_CURRENT_SOURCE_DIR}/deps/iPlug2/iPlug2.cmake)
find_package(iPlug2)

add_subdirectory(deps/eigen)
add_subdirectory(libs/json)
add_subdirectory(libs/AudioDSPTools)

add_subdirectory(plugin)