
add_library(fwk_voice_test_shared_test_utils INTERFACE)

target_sources(fwk_voice_test_shared_test_utils
    INTERFACE
        pseudo_rand/pseudo_rand.c
        testing/testing_misc.c
        testing/floating_fft.c)

target_include_directories(fwk_voice_test_shared_test_utils INTERFACE pseudo_rand testing)

add_library(fwk_voice::test::shared::test_utils ALIAS fwk_voice_test_shared_test_utils)

#################

add_library(fwk_voice_test_shared_profile_xcore INTERFACE)

target_sources(fwk_voice_test_shared_profile_xcore INTERFACE profile/profile.c)

target_include_directories(fwk_voice_test_shared_profile_xcore INTERFACE profile)

add_library(fwk_voice::test::shared::profile_xcore ALIAS fwk_voice_test_shared_profile_xcore)

#################

add_library(fwk_voice_test_shared_unity INTERFACE)

file( GLOB UNITY_SOURCES
    ${UNITY_PATH}/src/*.c)
target_sources(fwk_voice_test_shared_unity INTERFACE ${UNITY_SOURCES})

target_include_directories(fwk_voice_test_shared_unity
    INTERFACE
        ${UNITY_PATH}/src
        ${UNITY_PATH}/extras/fixture/src
        ${UNITY_PATH}/extras/memory/src)

target_compile_definitions(fwk_voice_test_shared_unity
    INTERFACE
        -DUNITY_SUPPORT_64)

add_library(fwk_voice::test::shared::unity ALIAS fwk_voice_test_shared_unity)

#################
add_subdirectory(legacy_compat)

#################

add_subdirectory(file_utils)
