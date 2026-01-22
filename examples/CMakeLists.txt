cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(lib_voice_examples)

add_subdirectory(app_aec)
add_subdirectory(app_vnr)

if(NOT BUILD_NATIVE)
    add_subdirectory(app_pipeline)
endif()
