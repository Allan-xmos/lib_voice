
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(fwk_voice_vnr_test_profile)

target_sources(fwk_voice_vnr_test_profile PRIVATE src/vnr_task.c)

target_link_libraries(fwk_voice_vnr_test_profile 
    PUBLIC
        fwk_voice::vnr
        fwk_voice::example::fileutils
        )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(fwk_voice_vnr_test_profile PRIVATE src/main.xc)
    target_link_libraries(fwk_voice_vnr_test_profile 
        PUBLIC
            fwk_voice::test::shared::profile_xcore
            )
    set(APP_COMPILER_FLAGS
        -Os
        -g
        -report
        -fxscope
        -mcmodel=large
        -Wno-xcore-fptrgroup    
        -target=${XCORE_TARGET}
    )
    target_compile_options(fwk_voice_vnr_test_profile
        PRIVATE ${APP_COMPILER_FLAGS} ${CONFIG_XSCOPE_PATH}/config.xscope)

    target_compile_definitions(fwk_voice_vnr_test_profile
        PRIVATE PROFILE_PROCESSING=1)

    target_link_options(fwk_voice_vnr_test_profile
        PRIVATE
            -w            
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope")
else()
    set(CMAKE_CXX_FLAGS "-std=c++11" CACHE STRING "C++ Compiler Base Flags" FORCE)

    set(BUILD_FLAGS
      "-Os"
      "-D__xtflm_conf_h_exists__"
      "-DNN_USE_REF"
    )
    target_compile_options(fwk_voice_vnr_test_profile
        PRIVATE ${BUILD_FLAGS})

    target_link_options(fwk_voice_vnr_test_profile
        PRIVATE
            ${BUILD_FLAGS})

    target_link_libraries(fwk_voice_vnr_test_profile PRIVATE "-lpthread")
    target_link_libraries(fwk_voice_vnr_test_profile PRIVATE m)

endif()

