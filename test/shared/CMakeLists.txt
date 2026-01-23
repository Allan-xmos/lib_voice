#################

add_library(fwk_voice_test_shared_profile_xcore INTERFACE)

target_sources(fwk_voice_test_shared_profile_xcore INTERFACE profile/profile.c)

target_include_directories(fwk_voice_test_shared_profile_xcore INTERFACE profile)

add_library(fwk_voice::test::shared::profile_xcore ALIAS fwk_voice_test_shared_profile_xcore)

#################
add_subdirectory(legacy_compat)

#################

add_subdirectory(file_utils)
