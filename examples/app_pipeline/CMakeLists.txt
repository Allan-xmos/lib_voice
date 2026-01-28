cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(app_pipeline)

set(APP_HW_TARGET XK-EVK-XU316)
set(APP_COMPILER_FLAGS_std_arch -report)
set(APP_COMPILER_FLAGS_alt_arch -report -DALT_ARCH_MODE=1)

set(APP_DEPENDENT_MODULES lib_voice)

set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)

XMOS_REGISTER_APP()
