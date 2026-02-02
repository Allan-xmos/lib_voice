
function(generate_schedule TARGET AEC_SCHEDULE_CONFIG)
    find_program(PYTHON_EXE python NO_CACHE)
    if( NOT PYTHON_EXE )
        message(FATAL_ERROR "Python not found for running aec schedule generation script generate_task_distribution_scheme.py")
    endif()

    set(GEN_SCHEDULE_SCRIPT "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tests/shared/python/generate_task_distribution_scheme.py")
    set( AUTOGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_src.autogen )
    set( AUTOGEN_SOURCES ${AUTOGEN_DIR}/aec_task_distribution.c )
    set( AUTOGEN_INCLUDES ${AUTOGEN_DIR}/aec_conf.h)

    set( GEN_SCHEDULE_SCRIPT_BYPRODUCTS ${AUTOGEN_SOURCES} ${AUTOGEN_INCLUDES} )

    unset(GEN_SCHEDULE_SCRIPT_ARGS)
    list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --out-dir ${AUTOGEN_DIR})
    list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --config ${AEC_SCHEDULE_CONFIG})

    add_custom_command(
        OUTPUT ${GEN_SCHEDULE_SCRIPT_BYPRODUCTS}
        BYPRODUCTS ${GEN_SCHEDULE_SCRIPT_BYPRODUCTS}
        COMMAND ${PYTHON_EXE} ${GEN_SCHEDULE_SCRIPT} ${GEN_SCHEDULE_SCRIPT_ARGS}
        COMMENT "Generating target ${TARGET} task distribution and top level config"
        DEPENDS ${GEN_SCHEDULE_SCRIPT}
        VERBATIM
        COMMAND_EXPAND_LISTS)


    target_sources(${TARGET} PRIVATE ${AUTOGEN_SOURCES})

    target_include_directories(${TARGET} PRIVATE ${AUTOGEN_DIR})

    target_compile_definitions(${TARGET} PRIVATE __aec_conf_h_exists__)

endfunction()

function(aec_collect_schedules out_single out_multi_names out_multi_scheds)
    #[[
    Collect AEC schedule configuration variables from the current CMake directory scope.

    This scans all directory-level variables for names matching:
    - AEC_SCHEDULE_CONFIG: Single schedule applied to all targets.
    - AEC_SCHEDULE_CONFIG_<config>: Multiple schedules keyed by target suffix `<config>`.

    Notes
    -----

    When using the `_config` suffix (i.e., `AEC_SCHEDULE_CONFIG_<config>`), the `<config>` must
    match the suffix used in corresponding `APP_COMPILER_FLAGS_<config>`,
    so that target names ending with `<config>` correctly receive the intended schedule.

    Parameters
    ----------
    out_single : str
        Name of the CMake variable to receive a list with zero or one schedule
        from `AEC_SCHEDULE_CONFIG`. Set in the parent scope.
    out_multi_names : str
        Name of the CMake variable to receive a list of config keys extracted
        from variables `AEC_SCHEDULE_CONFIG_<config>`. Set in the parent scope.
    out_multi_scheds : str
        Name of the CMake variable to receive a list of schedules corresponding
        to `out_multi_names`. Set in the parent scope.

    Returns
    -------
    None
        Results are assigned to the provided variable names in the parent scope.
    ]]
    # Scan directory variables for AEC_SCHEDULE_CONFIG*
    get_property(_vars DIRECTORY PROPERTY VARIABLES)
    list(REMOVE_DUPLICATES _vars)

    set(_single "")
    set(_multi_names "")
    set(_multi_scheds "")

    foreach(x ${_vars})
        string(FIND "${x}" "AEC_SCHEDULE_CONFIG" _found)
        if(NOT "${_found}" STREQUAL "-1")
            message(VERBOSE "Found AEC_SCHEDULE_CONFIG variable: ${x}")
            if("${x}" MATCHES "^AEC_SCHEDULE_CONFIG_[A-Za-z0-9_]+$")
                string(REGEX REPLACE "^AEC_SCHEDULE_CONFIG_([A-Za-z0-9_]+)$" "\\1" r_match "${x}")
                message(VERBOSE "  id='${r_match}' value='${${x}}'")
                list(APPEND _multi_names "${r_match}")
                list(APPEND _multi_scheds "${${x}}")
            elseif("${x}" STREQUAL "AEC_SCHEDULE_CONFIG")
                list(APPEND _single "${${x}}")
            endif()
        endif()
    endforeach()

    list(LENGTH _single _single_len)
    list(LENGTH _multi_names _multi_len)

    ## Sanity checks
    if(_single_len GREATER 1)
        message(FATAL_ERROR "Only one AEC_SCHEDULE_CONFIG allowed. Found: ${_single}")
    endif()

    if(_single_len GREATER 0 AND _multi_len GREATER 0)
        message(FATAL_ERROR "AEC_SCHEDULE_CONFIG and AEC_SCHEDULE_CONFIG_<config> provided at the same time")
    endif()

    message(VERBOSE "singleconfig_sched = ${_single}")
    message(VERBOSE "multipleconfig_name = ${_multi_names}")
    message(VERBOSE "multipleconfig_sched = ${_multi_scheds}")

    set(${out_single} "${_single}" PARENT_SCOPE)
    set(${out_multi_names} "${_multi_names}" PARENT_SCOPE)
    set(${out_multi_scheds} "${_multi_scheds}" PARENT_SCOPE)
endfunction()
