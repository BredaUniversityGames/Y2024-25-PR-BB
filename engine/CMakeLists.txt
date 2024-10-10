add_library(Engine INTERFACE)

# THIS FUNCTION DECLARES ALL THE DEFAULTS OF A MODULE
function(module_default_init module)
    target_link_libraries(${module} PRIVATE ProjectSettings)
    file(GLOB_RECURSE public_files CONFIGURE_DEPENDS "public/*.hpp")
    file(GLOB_RECURSE private_files CONFIGURE_DEPENDS "private/*.hpp" "private/*.cpp")
    target_sources(${module} PUBLIC ${public_files} PRIVATE ${private_files})
    target_include_directories(${module} PUBLIC "public" PRIVATE "private")
endfunction()

# THIS FUNCTION ADDS A MODULE TO THE MAIN ENGINE LIBRARY
function(target_add_module target directory module)
    message(STATUS "### - ${module} Module")
    add_subdirectory(${directory})
    target_link_libraries(${target} INTERFACE ${module})
endfunction()

# TODO: Replace with engine modules
add_library(OldProject)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "source/*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/*.hpp")

target_sources(OldProject PUBLIC ${headers} PRIVATE ${sources})
target_include_directories(OldProject PUBLIC "include" "../external")

option(ENABLE_PCH "Compile with precompiled header" ON)
if (ENABLE_PCH)
    target_precompile_headers(OldProject PRIVATE [["pch.hpp"]])
endif ()

target_link_libraries(OldProject PUBLIC Application PUBLIC External)
target_link_libraries(Engine INTERFACE OldProject INTERFACE External)

# ENGINE MODULES

message(STATUS "### Linking Engine Modules:")
target_add_module(Engine core Core)
target_add_module(Engine utility Utility)
target_add_module(Engine application Application)

# TODO: Replace with reusing precompiled headers


