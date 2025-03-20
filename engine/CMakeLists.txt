add_library(Engine INTERFACE)

if (ENABLE_PCH)
    add_subdirectory(pch)
endif ()

# Define if it's the distribution config for all modules
if ("${CMAKE_PRESET_NAME}" STREQUAL "x64-Distribution" OR "${CMAKE_PRESET_NAME}" STREQUAL "WSL-Distribution")
    add_compile_definitions(DISTRIBUTION)
endif()

# ENGINE MODULES

message(STATUS "### Linking Engine Modules:")
target_add_module(Engine core Core)
target_add_module(Engine utility Utility)
target_add_module(Engine application Application)
target_add_module(Engine particles Particles)
target_add_module(Engine scripting Scripting)
target_add_module(Engine renderer Renderer)
target_add_module(Engine physics Physics)
target_add_module(Engine pathfinding Pathfinding)
target_add_module(Engine steam Steam)
target_add_module(Engine audio Audio)
target_add_module(Engine user_interface UserInterface)
target_add_module(Engine bindings Bindings)
target_add_module(Engine inspector Inspector)
target_add_module(Engine game Game)
target_add_module(Engine ecs ECS)
target_add_module(Engine model_loading ModelLoading)
target_add_module(Engine thread Thread)
target_add_module(Engine settings Settings)
