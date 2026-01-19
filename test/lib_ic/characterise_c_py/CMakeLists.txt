## executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

#######
add_executable(fwk_voice_characterise_c_py )

target_link_libraries(fwk_voice_characterise_c_py 
    PUBLIC
        fwk_voice::example::test_wav_ic
        )

target_compile_definitions(fwk_voice_characterise_c_py 
    PRIVATE
        DISABLE_ADAPTION_CONTROLLER=1)
