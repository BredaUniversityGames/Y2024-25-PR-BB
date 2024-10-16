# SETS EXECUTABLE TO OUTPUT TO A SPECIFIC DIRECTORY
function(target_output_dir executable directory)
    set_target_properties(${executable} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${directory}
    )
endfunction()

# FUNCTION THAT DECLARES ALL THE DEFAULTS OF A MODULE
function(module_default_init module)

    target_link_libraries(${module} PRIVATE ProjectSettings)

    file(GLOB_RECURSE public_files CONFIGURE_DEPENDS "public/*.hpp")
    file(GLOB_RECURSE private_files CONFIGURE_DEPENDS "private/*.hpp" "private/*.cpp")

    target_sources(${module} PUBLIC ${public_files} PRIVATE ${private_files})
    target_include_directories(${module} PUBLIC "public" PRIVATE "private")

    if (ENABLE_PCH)
        target_link_libraries(${module} PRIVATE PCH)
        target_precompile_headers(${module} REUSE_FROM PCH)
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