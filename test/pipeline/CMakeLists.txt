
### Add interface library
add_library(pipeline_multi_thread_interface INTERFACE)
file( GLOB_RECURSE SOURCES_APP  src/*.c )

target_sources(pipeline_multi_thread_interface
    INTERFACE
        ${SOURCES_APP})

target_include_directories(pipeline_multi_thread_interface
    INTERFACE
        src)

target_link_libraries(pipeline_multi_thread_interface
    INTERFACE
        fwk_voice::aec
        fwk_voice::adec
        fwk_voice::ns
        fwk_voice::agc
        fwk_voice::ic
        fwk_voice::stage1
        fwk_voice::example::fileutils
        )

target_compile_definitions(pipeline_multi_thread_interface
    INTERFACE
        NUM_AEC_THREADS=2
    )

target_sources(pipeline_multi_thread_interface
    INTERFACE src/main.xc)

target_compile_options(pipeline_multi_thread_interface
    INTERFACE "-target=${XCORE_TARGET}")

target_link_options(pipeline_multi_thread_interface
    INTERFACE
        "-target=${XCORE_TARGET}"
        "-report"
        "${CONFIG_XSCOPE_PATH}/config.xscope")

### Add executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
add_executable(pipeline_std_arch)

target_link_libraries(pipeline_std_arch
    PUBLIC
        pipeline_multi_thread_interface
        )


## Build AEC+IC+NS_AGC pipeline for testing against python.
## Everything other than initial delay estimation is run,
## which is currently not supported on the python pipeline
add_executable(pipeline_aec_ic_ns_agc)

target_link_libraries(pipeline_aec_ic_ns_agc
    PUBLIC
        pipeline_multi_thread_interface
    )

target_compile_definitions(pipeline_aec_ic_ns_agc
    PRIVATE
        DISABLE_INITIAL_DELAY_EST=1
    )

## Build Alt arch pipeline
add_executable(pipeline_alt_arch)

target_link_libraries(pipeline_alt_arch
    PUBLIC
        pipeline_multi_thread_interface
    )

target_compile_definitions(pipeline_alt_arch
    PRIVATE
        ALT_ARCH_MODE=1
    )
