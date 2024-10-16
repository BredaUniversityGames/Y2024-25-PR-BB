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
    # target_precompile_headers(OldEngine PRIVATE [["pch.hpp"]])
endif ()

target_link_libraries(OldEngine PUBLIC Application PUBLIC External PRIVATE PCH)
target_link_libraries(Engine INTERFACE OldEngine INTERFACE External)

# ENGINE MODULES

message(STATUS "### Linking Engine Modules:")
target_add_module(Engine core Core)
target_add_module(Engine utility Utility)
target_add_module(Engine application Application)



