
set( CONFIG_XSCOPE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/shared_src/etc )

add_subdirectory( shared_src )

add_subdirectory( aec )

add_subdirectory( pipeline_single_threaded )

add_subdirectory( vnr )

## Multi thread example builds only on XCORE
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)

    add_subdirectory( pipeline_multi_threaded )
endif()

add_subdirectory( pipeline_alt_arch )
