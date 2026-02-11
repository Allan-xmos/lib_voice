
# Selector to split tests into two parts for parallel build in Jenkinsfile
set(TEST_BUILD_PART "all" CACHE STRING "Which part of tests to build: all|partA|partB")
set_property(CACHE TEST_BUILD_PART PROPERTY STRINGS all partA partB)

## Factor by which to speed up unit tests
if(NOT DEFINED TEST_SPEEDUP_FACTOR)
set( TEST_SPEEDUP_FACTOR "1" CACHE STRING "Test speedup factor." )
endif()

if(NOT DEFINED DE_UNIT_TESTS_BUILD_CONFIG)
set(
    DE_UNIT_TESTS_BUILD_CONFIG
    "2 2 2 10 5"
    CACHE STRING
    "AEC build configuration for de_unit_tests in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format"
    )
endif()

if(NOT DEFINED TEST_ADEC_BUILD_CONFIG)
set(
    TEST_ADEC_BUILD_CONFIG
    "2 2 2 10 5"
    CACHE STRING
    "AEC build configuration for test_adec in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format"
    )
endif()

if(NOT DEFINED TEST_ADEC_STARTUP_BUILD_CONFIG)
set(
    TEST_ADEC_STARTUP_BUILD_CONFIG
    "2 2 2 10 5"
    CACHE STRING
    "AEC build configuration for test_adec_startup in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format"
    )
endif()

if(NOT DEFINED TEST_DELAY_ESTIMATOR_BUILD_CONFIG)
set(
    TEST_DELAY_ESTIMATOR_BUILD_CONFIG
    "2 2 2 10 5"
    CACHE STRING
    "AEC build configuration for test_delay_estimator in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format"
    )
endif()

if(NOT DEFINED TEST_WAV_ADEC_BUILD_CONFIG)
set(
    TEST_WAV_ADEC_BUILD_CONFIG
    "2 2 2 10 5"
    CACHE STRING
    "AEC build configuration for test_wav_adec in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format"
    )
endif()

if(NOT DEFINED AEC_UNIT_TESTS_BUILD_CONFIG)
set(
    AEC_UNIT_TESTS_BUILD_CONFIG
    "2 2 2 10 5"
    CACHE STRING
    "AEC build configuration for aec_unit_tests in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format"
    )
endif()

if(NOT DEFINED TEST_AEC_ENHANCEMENTS_BUILD_CONFIG)
set(
    TEST_AEC_ENHANCEMENTS_BUILD_CONFIG
    "2 2 2 10 5"
    CACHE STRING
    "AEC build configuration for test_aec_enhancements in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format"
    )
endif()

if(NOT DEFINED TEST_AEC_SPEC_BUILD_CONFIG)
set(
    TEST_AEC_SPEC_BUILD_CONFIG
    "2 1 1 20 10"
    CACHE STRING
    "AEC build configuration for test_aec_spec in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format"
    )
endif()
