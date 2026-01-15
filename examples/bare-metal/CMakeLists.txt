
set( CONFIG_XSCOPE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/shared_src/etc )

add_subdirectory( shared_src )

add_subdirectory( aec )

add_subdirectory( vnr )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_subdirectory( pipeline )
endif()
