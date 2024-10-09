add_library(Engine)

# TODO: Replace with engine modules
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "source/*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/*.hpp")

target_sources(Engine PUBLIC ${headers} PRIVATE ${sources})
target_include_directories(Engine PUBLIC "include" "../external")

# ENGINE MODULES

add_subdirectory(utility)
add_subdirectory(core)

target_link_libraries(Engine
        PRIVATE ProjectSettings
        PUBLIC External
        PUBLIC Utility
        PUBLIC Core
)

# TODO: Replace with reusing precompiled headers

option(ENABLE_PCH "Compile with precompiled header" ON)
if (ENABLE_PCH)
    target_precompile_headers(Engine PRIVATE [["pch.hpp"]])
endif ()

