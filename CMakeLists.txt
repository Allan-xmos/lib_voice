
option(USE_CUSTOM_CMAKE "Enable legacy custom CMake build" OFF)

if(USE_CUSTOM_CMAKE)
cmake_minimum_required(VERSION 3.21)

## Disable in-source build.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

project(fwk_voice)

## Add libs and apps
add_subdirectory( lib_voice )

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory( test )
endif()

else()
cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(fwk_voice)
add_subdirectory(test)
endif()
