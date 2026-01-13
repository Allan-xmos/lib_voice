
### Add interface library
add_library(fwk_voice_example_pipeline_single_thread_interface INTERFACE)

file( GLOB_RECURSE SOURCES_APP  src/*.c )
target_sources(fwk_voice_example_pipeline_single_thread_interface
    INTERFACE
        ${SOURCES_APP})

target_include_directories(fwk_voice_example_pipeline_single_thread_interface
    INTERFACE
        src)

target_link_libraries(fwk_voice_example_pipeline_single_thread_interface
    INTERFACE
        fwk_voice::aec
        fwk_voice::adec
        fwk_voice::ns
        fwk_voice::agc
        fwk_voice::ic
        fwk_voice::stage1
        fwk_voice::example::fileutils
        )

target_compile_definitions(fwk_voice_example_pipeline_single_thread_interface
    INTERFACE
        NUM_AEC_THREADS=1
    )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(fwk_voice_example_pipeline_single_thread_interface
        INTERFACE src/main.xc)

    target_compile_options(fwk_voice_example_pipeline_single_thread_interface
        INTERFACE "-target=${XCORE_TARGET}")

endif()

### Add executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(fwk_voice_example_bare_metal_pipeline_single_thread)

target_link_libraries(fwk_voice_example_bare_metal_pipeline_single_thread
    PRIVATE
        fwk_voice_example_pipeline_single_thread_interface
        )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_link_options(fwk_voice_example_bare_metal_pipeline_single_thread
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope")
elseif(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    # Static linking doesn't work on Mac(ld: library not found for -lcrt0.o)
    # Note we link this app statically so that we can run it on a different machine
    # that may not have all of the dependancies in the right places
    target_link_libraries(fwk_voice_example_bare_metal_pipeline_single_thread
        PRIVATE "-static")
endif()
