### PROJECT ###
cmake_minimum_required(VERSION 3.22.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(Project)

### HELPERS AND INCLUDES

include(FetchContent)
include(scripts/cmake/code-modules.cmake)

### COMPILATION SETTINGS ###

option(ENABLE_TESTS "Compile with Unit Tests" ON)
option(WARNINGS_AS_ERRORS "Enable warnings as errors" OFF)
option(ENABLE_PCH "Compile with precompiled header" ON)
option(COMPILE_SHADERS "Compile all GLSL shaders as part of build step" ON)
option(ENABLE_UNITY "Compile using Unity Builds" ON)

add_library(ProjectSettings INTERFACE)
target_compile_features(ProjectSettings INTERFACE cxx_std_20)

target_compile_options(ProjectSettings
        INTERFACE -Wall INTERFACE -Wextra INTERFACE -Wno-unknown-pragmas)

target_compile_options(ProjectSettings
        INTERFACE -Wall INTERFACE -Wextra INTERFACE -Wno-unknown-pragmas)

if (WARNINGS_AS_ERRORS)
    message(STATUS "### Warnings are enabled as Errors")
    target_compile_options(ProjectSettings INTERFACE -Werror)
endif ()

if (ENABLE_PCH)
    message(STATUS "### Compiling with Precompiled Headers")
endif ()

if (ENABLE_UNITY)
    message(STATUS "### Compiling with Unity (Jumbo) builds")
endif ()


# Add external dependencies
add_subdirectory(external)

# Add unit testing target executable
if (ENABLE_TESTS)
    add_subdirectory(tests)
endif ()

# Add engine library
add_subdirectory(engine)

# Add game executable (currently just main.cpp)
add_executable(Game "engine/main.cpp")
target_link_libraries(Game
        PRIVATE ProjectSettings
        PRIVATE Engine
)

### SHADERS COMPILE STEP ###
if (COMPILE_SHADERS)
    message(STATUS "### Shaders will be compiled on build")
    add_subdirectory(shaders)
    add_dependencies(Game Shaders)
endif ()

### COPY FILES ###

# DLLS

add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/external/fmod/lib/$<PLATFORM_ID>/$<CONFIG>"
        $<TARGET_FILE_DIR:Game>)

add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/external/steamworks/lib/$<PLATFORM_ID>"
        $<TARGET_FILE_DIR:Game>)

# Steam app ID (TODO: Remove from distribution build!)

add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${PROJECT_SOURCE_DIR}/steam_appid.txt"
        $<TARGET_FILE_DIR:Game>)
