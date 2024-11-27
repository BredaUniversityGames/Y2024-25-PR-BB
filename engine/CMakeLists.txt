add_library(Engine INTERFACE)

if (ENABLE_PCH)
    add_subdirectory(pch)
endif ()

# TODO: Replace with engine modules
add_library(OldEngine)
module_default_init(OldEngine)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "source/*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/*.hpp")

target_sources(OldEngine PUBLIC ${headers} PRIVATE ${sources})
target_include_directories(OldEngine PUBLIC "include" "../external")
target_link_libraries(OldEngine PUBLIC Application PUBLIC Renderer PUBLIC External PUBLIC Physics)

source_exclude_unity("source/lib/stbi_impl.cpp")

# ENGINE MODULES

message(STATUS "### Linking Engine Modules:")
target_add_module(Engine core Core)
target_add_module(Engine utility Utility)
target_add_module(Engine application Application)
target_add_module(Engine scripting Scripting)
target_add_module(Engine renderer Renderer)
target_add_module(Engine physics Physics)
target_add_module(Engine steam Steam)

target_link_libraries(Engine INTERFACE OldEngine INTERFACE External)