include(${CMAKE_CURRENT_LIST_DIR}/model.cmake)

## VNR Inference
add_library(fwk_voice_module_lib_vnr STATIC)


file(GLOB_RECURSE VNR_C_SOURCES src/*.c)
file(GLOB_RECURSE VNR_CXX_SOURCES src/*.cpp)

target_sources(fwk_voice_module_lib_vnr 
    PRIVATE 
        ${MODEL_OUT_PATH}.cpp 
        ${VNR_CXX_SOURCES}
        ${VNR_C_SOURCES}
)

target_include_directories(fwk_voice_module_lib_vnr PUBLIC api)

target_link_libraries(fwk_voice_module_lib_vnr
    PUBLIC
        lib_xcore_math
)

target_link_libraries(fwk_voice_module_lib_vnr
    PRIVATE
	    tflite_micro
)

target_compile_definitions(fwk_voice_module_lib_vnr PRIVATE HEADROOM_CHECK=0)


target_compile_options(fwk_voice_module_lib_vnr
    PRIVATE
        -Os
        -g
)

add_library(fwk_voice::vnr ALIAS fwk_voice_module_lib_vnr)
