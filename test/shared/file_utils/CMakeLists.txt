### Fetch xscope_fileio
set(XSCOPE_FILEIO_PATH ${CMAKE_BINARY_DIR}/fwk_voice_deps)

include(FetchContent)

FetchContent_Declare(
    xscope_fileio
    GIT_REPOSITORY https://github.com/xmos/xscope_fileio.git
    GIT_TAG        v1.3.1
    GIT_SHALLOW    FALSE
    SOURCE_DIR     ${XSCOPE_FILEIO_PATH}/xscope_fileio
)
FetchContent_Populate(xscope_fileio)

### xscope_fileio target
set(APP_NAME fwk_voice_deps_xscope_fileio)
add_library(${APP_NAME}  INTERFACE)

target_sources(${APP_NAME}
    INTERFACE
        ${XSCOPE_FILEIO_PATH}/xscope_fileio/xscope_fileio/src/xscope_io_device.c)

target_include_directories(${APP_NAME}
    INTERFACE
        ${XSCOPE_FILEIO_PATH}/xscope_fileio/xscope_fileio
        ${XSCOPE_FILEIO_PATH}/xscope_fileio/xscope_fileio/api)

add_library(fwk_voice_deps::xscope_fileio ALIAS fwk_voice_deps_xscope_fileio)

add_library(fwk_voice_example_shared_src_file_utils INTERFACE)
target_sources(fwk_voice_example_shared_src_file_utils
    INTERFACE
        src/fileio.c
        src/wav_utils.c
)
target_include_directories(fwk_voice_example_shared_src_file_utils
    INTERFACE
        src
)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_link_libraries(fwk_voice_example_shared_src_file_utils
        INTERFACE
            fwk_voice_deps::xscope_fileio)
    target_compile_definitions(fwk_voice_example_shared_src_file_utils
        INTERFACE
            TEST_WAV_XSCOPE=1)
endif()

add_library(fwk_voice::example::fileutils ALIAS fwk_voice_example_shared_src_file_utils)
