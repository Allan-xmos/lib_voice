
############################
add_library(fwk_voice_example_shared_ic_test_wav INTERFACE)
target_sources(fwk_voice_example_shared_ic_test_wav
    INTERFACE
        src/ic_test_task.c)
target_link_libraries(fwk_voice_example_shared_ic_test_wav
    INTERFACE
        fwk_voice::ic
        fwk_voice::example::fileutils
        )
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(fwk_voice_example_shared_ic_test_wav
        INTERFACE
            src/main.xc
    )
    target_link_libraries(fwk_voice_example_shared_ic_test_wav
        INTERFACE
            fwk_voice::example::profile_xcore
            )
    target_compile_options(fwk_voice_example_shared_ic_test_wav
        INTERFACE "-target=${XCORE_TARGET}")

    target_link_options(fwk_voice_example_shared_ic_test_wav
        INTERFACE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    target_link_libraries(fwk_voice_example_shared_ic_test_wav
        INTERFACE m)
endif()
add_library(fwk_voice::example::test_wav_ic ALIAS fwk_voice_example_shared_ic_test_wav)
