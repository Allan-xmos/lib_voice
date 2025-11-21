
add_subdirectory(vnr_unit_tests)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_subdirectory(profile_memory)
endif()
