
## Defines for tests

## The unit test apps
if(NOT USE_CUSTOM_CMAKE)
    cmake_minimum_required(VERSION 3.21)
    include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
    project(fwk_voice_tests_aec)
    add_subdirectory(test_aec_xccm)
    add_subdirectory( test_aec_enhancements )
    add_subdirectory( test_aec_spec )
    if(NOT BUILD_NATIVE)
        add_subdirectory( aec_unit_tests )
    endif()
endif()
