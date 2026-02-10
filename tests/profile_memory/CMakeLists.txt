cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(app_memory)
set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)

set(APP_HW_TARGET XK-EVK-XU316)

set(COMPILER_FLAGS -Os
                   -g
                   -report)

set(APP_COMPILER_FLAGS_aec_std_arch
            ${COMPILER_FLAGS}
            -DAEC=1)
set(AEC_SCHEDULE_CONFIG_aec_std_arch "1 2 2 10 5")

set(APP_COMPILER_FLAGS_aec_alt_arch
            ${COMPILER_FLAGS}
            -DAEC=1)
set(AEC_SCHEDULE_CONFIG_aec_alt_arch "1 1 2 15 5")

set(APP_COMPILER_FLAGS_ic
            ${COMPILER_FLAGS}
            -DIC=1)

set(APP_COMPILER_FLAGS_vnr
            ${COMPILER_FLAGS}
            -DVNR=1)

set(APP_COMPILER_FLAGS_agc
            ${COMPILER_FLAGS}
            -DAGC=1)

set(APP_COMPILER_FLAGS_agc
            ${COMPILER_FLAGS}
            -DAGC=1)

set(APP_COMPILER_FLAGS_ns
            ${COMPILER_FLAGS}
            -DNS=1)

set(APP_COMPILER_FLAGS_adec
            ${COMPILER_FLAGS}
            -DADEC=1)

set(APP_COMPILER_FLAGS_empty
            ${COMPILER_FLAGS})

set(APP_DEPENDENT_MODULES lib_voice)

XMOS_REGISTER_APP()
