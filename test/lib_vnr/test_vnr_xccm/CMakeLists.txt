cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(test_vnr_xccm)

set(APP_HW_TARGET XK-EVK-XU316)
set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../)
set(APP_DEPENDENT_MODULES "lib_vnr")

set(XMOS_DEP_DIR_lib_vnr ${XMOS_SANDBOX_DIR}/lib_voice/modules)

XMOS_REGISTER_APP()
