
### Add interface library
add_library(fwk_voice_example_pipeline_interface INTERFACE)

file( GLOB_RECURSE SOURCES_APP  src/*.c )
target_sources(fwk_voice_example_pipeline_interface
    INTERFACE
        ${SOURCES_APP})

target_include_directories(fwk_voice_example_pipeline_interface
    INTERFACE
        src)

target_link_libraries(fwk_voice_example_pipeline_interface
    INTERFACE
        fwk_voice::aec
        fwk_voice::adec
        fwk_voice::ns
        fwk_voice::agc
        fwk_voice::ic
        fwk_voice::stage1
        )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(fwk_voice_example_pipeline_interface
        INTERFACE src/main.xc)

    target_compile_options(fwk_voice_example_pipeline_interface
        INTERFACE "-target=${XCORE_TARGET}")

    target_link_options(fwk_voice_example_pipeline_interface
        INTERFACE
            "-target=${XCORE_TARGET}"
            "-report"
            )
endif()

### Add executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(fwk_voice_example_pipeline_std_arch)

target_link_libraries(fwk_voice_example_pipeline_std_arch
    PRIVATE
        fwk_voice_example_pipeline_interface
        )

####### alt arch pipeline
add_executable(fwk_voice_example_pipeline_alt_arch)

target_link_libraries(fwk_voice_example_pipeline_alt_arch
    PUBLIC
        fwk_voice_example_pipeline_interface
        )

target_compile_definitions(fwk_voice_example_pipeline_alt_arch
    PRIVATE
        ALT_ARCH_MODE=1
    )
