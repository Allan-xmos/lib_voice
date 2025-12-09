set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

################################
add_executable(fwk_voice_example_bare_metal_vnr)

target_sources(fwk_voice_example_bare_metal_vnr PUBLIC src/main.c)

target_link_libraries(fwk_voice_example_bare_metal_vnr
    PUBLIC 
        fwk_voice::vnr
)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_options(fwk_voice_example_bare_metal_vnr
        PRIVATE
            "-target=${XCORE_TARGET}"
    )
    target_link_options(fwk_voice_example_bare_metal_vnr
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
    )
else()
    target_link_libraries(fwk_voice_example_bare_metal_vnr
        PRIVATE
            m
    )
endif()
