
add_subdirectory( aec )

add_subdirectory( vnr )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_subdirectory( pipeline )
endif()
