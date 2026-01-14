#file( GLOB_RECURSE    LIB_AEC_SOURCES    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c )

#set(CMAKE_C_CLANG_TIDY "clang-tidy;-checks=*")

add_library(fwk_voice_module_aec INTERFACE)

include(schedule.cmake)

target_sources(fwk_voice_module_aec
    INTERFACE
        src/aec_impl.c
        src/aec_l2_impl.c
        src/aec_priv_impl.c
        src/aec_process_frame.c
        src/aec_schedule_defaults.c
)

target_include_directories(fwk_voice_module_aec
    INTERFACE
        api
        src ## this is needed for the IC
)

target_compile_options(fwk_voice_module_aec
    INTERFACE
        -Os
        -g
)

target_link_libraries(fwk_voice_module_aec
    INTERFACE
        lib_xcore_math
)

## Create an alias
add_library(fwk_voice::aec ALIAS fwk_voice_module_aec)

