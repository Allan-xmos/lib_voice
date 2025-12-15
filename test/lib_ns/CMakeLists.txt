add_subdirectory( test_ns_profile )
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_subdirectory( ns_unit_tests )
endif()

