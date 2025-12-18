## executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include(basic_ic_app.cmake)
#######
add_executable(fwk_voice_test_ic_profile)

target_link_libraries(fwk_voice_test_ic_profile
    PUBLIC
        fwk_voice::example::test_wav_ic
        )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_definitions(fwk_voice_test_ic_profile
        PRIVATE
            PROFILE_PROCESSING=1)
endif()
