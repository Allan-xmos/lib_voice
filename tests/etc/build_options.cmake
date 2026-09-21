
# Selector to split tests into two parts for parallel build in Jenkinsfile
set(TEST_BUILD_PART "all" CACHE STRING "Which part of tests to build: all|partA|partB")
set_property(CACHE TEST_BUILD_PART PROPERTY STRINGS all partA partB)

## Factor by which to speed up unit tests
if(NOT DEFINED TEST_SPEEDUP_FACTOR)
set( TEST_SPEEDUP_FACTOR "1" CACHE STRING "Test speedup factor." )
endif()

# The ADEC suites below declare 15 main filter phases rather than 10, because that is what they actually ask
# aec_init() for. The delay estimation configuration in test_bin_adec/src/test_bin.c, and the aec_init() calls in
# de_unit_tests, request 30 phases on a single x channel, which needs 30 X FIFO phases. The pools provide
# AEC_MAX_X_CHANNELS * AEC_MAIN_FILTER_PHASES of those, so AEC_MAIN_FILTER_PHASES has to be at least 15.
#
# This used to balance at 10 only because a filter phase and an X FIFO phase were the same size, so trading y
# channels for phases was 1:1. A time domain filter phase is a quarter the size of an X FIFO phase, so that trade
# no longer pays for itself and the declared maximum has to cover the real demand. aec_init() now asserts it.
if(NOT DEFINED DE_UNIT_TESTS_BUILD_CONFIG)
set(
    DE_UNIT_TESTS_BUILD_CONFIG
    "2 2 2 15 5"
    CACHE STRING
    "AEC build configuration for de_unit_tests in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format"
    )
endif()

if(NOT DEFINED TEST_ADEC_BUILD_CONFIG)
set(
    TEST_ADEC_BUILD_CONFIG
    "2 2 2 15 5"
    CACHE STRING
    "AEC build configuration for test_adec in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format"
    )
endif()

if(NOT DEFINED TEST_ADEC_STARTUP_BUILD_CONFIG)
set(
    TEST_ADEC_STARTUP_BUILD_CONFIG
    "2 2 2 15 5"
    CACHE STRING
    "AEC build configuration for test_adec_startup in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format"
    )
endif()

if(NOT DEFINED TEST_DELAY_ESTIMATOR_BUILD_CONFIG)
set(
    TEST_DELAY_ESTIMATOR_BUILD_CONFIG
    "2 2 2 15 5"
    CACHE STRING
    "AEC build configuration for test_delay_estimator in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format"
    )
endif()

if(NOT DEFINED TEST_BIN_ADEC_BUILD_CONFIG)
set(
    TEST_BIN_ADEC_BUILD_CONFIG
    "2 2 2 15 5"
    CACHE STRING
    "AEC build configuration for test_bin_adec in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format"
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
