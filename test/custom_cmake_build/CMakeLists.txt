### Add executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(pipeline_custom_cmake)
file( GLOB_RECURSE SOURCES_APP  src/*.c )

target_sources(pipeline_custom_cmake
    PUBLIC
        ${SOURCES_APP})

target_include_directories(pipeline_custom_cmake
    PUBLIC
        src)

generate_schedule(pipeline_custom_cmake "1 2 2 10 5")

target_link_libraries(pipeline_custom_cmake
    PUBLIC
        fwk_voice::aec
        fwk_voice::adec
        fwk_voice::ns
        fwk_voice::agc
        fwk_voice::ic
        fwk_voice::stage1)

set(COMMON_OPTIONS
                    -Os
                    -MMD
                    -g
                    -Wno-format
                    -Wall)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_options(pipeline_custom_cmake
        PRIVATE
            -target=XK-EVK-XU316
            ${COMMON_OPTIONS})

    target_link_options(pipeline_custom_cmake
        PRIVATE
            -target=XK-EVK-XU316
            -report)
else()
    target_compile_options(pipeline_custom_cmake
        PRIVATE
            ${COMMON_OPTIONS})
endif()
