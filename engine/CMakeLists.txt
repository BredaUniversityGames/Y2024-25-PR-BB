add_library(Engine)
target_link_libraries(Engine PRIVATE ProjectSettings)

# target_add_module(Engine "directory" Module)
function(target_add_module target directory module)
    message(STATUS "### - ${module} Module")
    add_subdirectory(${directory})
    target_link_libraries(${target} PUBLIC ${module})
endfunction()

# TODO: Replace with engine modules
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "source/*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/*.hpp")

target_sources(Engine PUBLIC ${headers} PRIVATE ${sources})
target_include_directories(Engine PUBLIC "include" "../external")
target_link_libraries(Engine PUBLIC External)

# ENGINE MODULES

message(STATUS "### Linking Engine Modules:")
target_add_module(Engine core Core)
target_add_module(Engine utility Utility)
# target_add_module(Engine application Application)

# TODO: Replace with reusing precompiled headers

option(ENABLE_PCH "Compile with precompiled header" ON)
if (ENABLE_PCH)
    target_precompile_headers(Engine PRIVATE [["pch.hpp"]])
endif ()

