
add_subdirectory(external_deps)

######
add_library(fwk_voice_example_shared_src_file_utils INTERFACE)
target_sources(fwk_voice_example_shared_src_file_utils
    INTERFACE
        file_utils/fileio.c
        file_utils/wav_utils.c
)
target_include_directories(fwk_voice_example_shared_src_file_utils
    INTERFACE
        file_utils
)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_link_libraries(fwk_voice_example_shared_src_file_utils
        INTERFACE
            fwk_voice_deps::xscope_fileio)
endif()

add_library(fwk_voice::example::fileutils ALIAS fwk_voice_example_shared_src_file_utils)
