# SETS EXECUTABLE TO OUTPUT TO A SPECIFIC DIRECTORY
function(target_output_dir executable directory)
    set_target_properties(${executable} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${directory}
    )
endfunction()

# ENABLES UNITY BUILDS ON TARGETS
function(target_enable_unity target)
    set_target_properties(${target} PROPERTIES UNITY_BUILD ON)
    set_target_properties(${target} PROPERTIES UNITY_BUILD_BATCH_SIZE 4)
endfunction()

# DISABLES UNITY BUILD ON A SPECIFIC FILE
function(source_exclude_unity file)
    set_source_files_properties(${file} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
endfunction()

# DISABLES PRECOMPILED HEADER ON SOURCE FILES
function(source_disable_pch files)
    set_source_files_properties(${file} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endfunction()

# FUNCTION THAT DECLARES ALL THE DEFAULTS OF A MODULE
function(module_default_init module)

    file(GLOB_RECURSE public_headers CONFIGURE_DEPENDS "public/*.hpp")
    file(GLOB_RECURSE private_sources CONFIGURE_DEPENDS "private/*.cpp")
    file(GLOB_RECURSE private_headers CONFIGURE_DEPENDS "private/*.hpp")

    target_sources(${module} PUBLIC ${public_files} PRIVATE ${private_headers} PRIVATE ${private_sources})
    target_include_directories(${module} PUBLIC "public" PRIVATE "private")

    target_link_libraries(${module} PRIVATE ProjectSettings)

    if (ENABLE_PCH)
        target_link_libraries(${module} PRIVATE PCH)
        target_precompile_headers(${module} REUSE_FROM PCH)
    endif ()

    if (COMPILE_TESTS)
        file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "tests/*.cpp")

        if (test_sources)
            target_sources(UnitTests PRIVATE ${test_sources})
            target_link_libraries(UnitTests PRIVATE ${module})
            target_include_directories(UnitTests PRIVATE "private")
        endif ()

    endif ()

    if (ENABLE_UNITY)
        target_enable_unity(${module})
    endif ()

endfunction()

# FUNCTION THAT ADDS A MODULE TO THE MAIN ENGINE LIBRARY
function(target_add_module target directory module)
    message(STATUS "### - ${module} Module")
    add_subdirectory(${directory})
    target_link_libraries(${target} INTERFACE ${module})
endfunction()

# MACRO THAT OVERRIDES CACHED OPTIONS IN CMAKE
macro(override_option option_name option_value)
    set(${option_name} ${option_value} CACHE BOOL "" FORCE)
endmacro()