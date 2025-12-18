set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(fwk_voice_test_ns_profile)

target_sources(fwk_voice_test_ns_profile PRIVATE src/ns_app.c)

target_link_libraries(fwk_voice_test_ns_profile
    PUBLIC
        fwk_voice::ns
        fwk_voice::example::fileutils
    )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(fwk_voice_test_ns_profile PRIVATE src/main.xc)

    target_link_libraries(fwk_voice_test_ns_profile
        PRIVATE
            fwk_voice::example::profile_xcore
    )
    target_compile_options(fwk_voice_test_ns_profile
        PRIVATE
            "-DPROFILE_PROCESSING=1"
            "-target=${XCORE_TARGET}")

    target_link_options(fwk_voice_test_ns_profile
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    target_link_libraries(fwk_voice_test_ns_profile
        PRIVATE 
            # need to explicitly link m after lib_xcore_math
            lib_xcore_math
            m)
endif()
