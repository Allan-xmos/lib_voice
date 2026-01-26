### Add executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")

add_executable(pipeline_custom_cmake)
file( GLOB_RECURSE SOURCES_APP  src/*.c )

target_sources(pipeline_custom_cmake
    PUBLIC
        ${SOURCES_APP})

target_include_directories(pipeline_custom_cmake
    PUBLIC
        src)

target_link_libraries(pipeline_custom_cmake
    PUBLIC
        fwk_voice::aec
        fwk_voice::adec
        fwk_voice::ns
        fwk_voice::agc
        fwk_voice::ic
        fwk_voice::stage1)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(pipeline_custom_cmake
        PUBLIC src/main.xc)

    target_compile_options(pipeline_custom_cmake
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(pipeline_custom_cmake
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report")
else()
    #message(STATUS "IN X86 COMPILE")
    #target_compile_features(pipeline_custom_cmake PRIVATE cxx_std_11)

    target_compile_definitions(pipeline_custom_cmake
        PRIVATE
            X86_BUILD=1)
endif()
