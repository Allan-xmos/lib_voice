
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(THREADS_CONF 1 2)

foreach (BUILD_CONFIG ${THREADS_CONF})
    set(EXAMPLE_NAME fwk_voice_example_aec_${BUILD_CONFIG}th)

    add_executable(${EXAMPLE_NAME})

    target_sources(${EXAMPLE_NAME} PUBLIC src/main.c)

    target_link_libraries(${EXAMPLE_NAME}
        PUBLIC
            fwk_voice::aec
    )
    target_compile_options(${EXAMPLE_NAME}
        PRIVATE
            "-DAEC_THREADS=${BUILD_CONFIG}"
    )

    if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
        target_compile_options(${EXAMPLE_NAME}
            PRIVATE
                "-target=${XCORE_TARGET}"
        )
        target_link_options(${EXAMPLE_NAME}
            PRIVATE
                "-target=${XCORE_TARGET}"
                "-report"
        )
    endif()
endforeach()
