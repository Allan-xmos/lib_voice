
# Selector to split tests into two parts for parallel build in Jenkinsfile
set(TEST_BUILD_PART "all" CACHE STRING "Which part of tests to build: all|partA|partB")
set_property(CACHE TEST_BUILD_PART PROPERTY STRINGS all partA partB)

## Factor by which to speed up unit tests
if(NOT DEFINED TEST_SPEEDUP_FACTOR)
set( TEST_SPEEDUP_FACTOR "1" CACHE STRING "Test speedup factor." )
endif()

# The ADEC tests drive the AEC with runtime configurations that differ from the compile time one:
# test_bin_adec's delay estimator mode always uses 1 y channel, 1 x channel and 30 main filter phases,
# and test_adec's normal mode uses 1 y channel, 2 x channels and 15 main filter phases. The AEC memory
# pool reserves a fixed number of phases for h_hat and for X_fifo separately, so the compile time
# configuration has to cover the largest num_x_channels * num_main_filter_phases used at runtime (30),
# i.e. AEC_MAX_X_CHANNELS * <num_main_phases> >= 30 - hence 15 main phases rather than 10. It is not
# enough for the total phase budget to fit, which is all the preconditions on aec_init() require; the
# pool size assertions in aec_priv_main_init()/aec_priv_shadow_init() check the real invariant.
#
# The y and x channel counts stay at the maximum because test_bin_adec derives its input and output
# wav channel layout from AEC_MAX_Y_CHANNELS/AEC_MAX_X_CHANNELS, so lowering them would change the
# test audio rather than just the memory footprint.
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
    "2 2 2 10 5"
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
