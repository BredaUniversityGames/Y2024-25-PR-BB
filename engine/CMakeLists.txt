# THIS FUNCTION DECLARES ALL THE DEFAULTS OF A MODULE
function(module_default_init module)
    target_link_libraries(${module} PRIVATE ProjectSettings)

    file(GLOB_RECURSE public_files CONFIGURE_DEPENDS "public/*.hpp")
    file(GLOB_RECURSE private_files CONFIGURE_DEPENDS "private/*.hpp" "private/*.cpp")

    target_sources(${module} PUBLIC ${public_files} PRIVATE ${private_files})
    target_include_directories(${module} PUBLIC "public" PRIVATE "private")

    if (ENABLE_PCH)
        target_precompile_headers(${module} REUSE_FROM PCH)
    endif ()
endfunction()

# THIS FUNCTION ADDS A MODULE TO THE MAIN ENGINE LIBRARY
function(target_add_module target directory module)
    message(STATUS "### - ${module} Module")
    add_subdirectory(${directory})
    target_link_libraries(${target} INTERFACE ${module})
endfunction()

add_library(Engine INTERFACE)

if (ENABLE_PCH)
    add_subdirectory(pch)
endif ()

# TODO: Replace with engine modules
add_library(OldEngine)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "source/*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/*.hpp")

target_sources(OldEngine PUBLIC ${headers} PRIVATE ${sources})
target_include_directories(OldEngine PUBLIC "include" "../external")
target_link_libraries(OldEngine PRIVATE External PRIVATE ProjectSettings)

if (ENABLE_PCH)
    # target_precompile_headers(OldEngine REUSE_FROM PCH)
    # target_precompile_headers(OldEngine PRIVATE [["pch.hpp"]])
endif ()

target_link_libraries(OldEngine PUBLIC Application PUBLIC External PRIVATE PCH)
target_link_libraries(Engine INTERFACE OldEngine INTERFACE External)

# ENGINE MODULES

message(STATUS "### Linking Engine Modules:")
target_add_module(Engine core Core)
target_add_module(Engine utility Utility)
target_add_module(Engine application Application)

# TODO: Replace with reusing precompiled headers


