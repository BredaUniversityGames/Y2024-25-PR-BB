### PROJECT ###
cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_BUILD_RPATH "$ORIGIN")

project(Project)

### HELPERS AND INCLUDES

include(FetchContent)
include(scripts/cmake/code-modules.cmake)

### COMPILATION SETTINGS ###

option(WARNINGS_AS_ERRORS "Enable warnings as errors" OFF)

option(COMPILE_TESTS "Compile with Unit Tests" ON)
option(COMPILE_SHADERS "Compile all GLSL shaders as part of build step" ON)

option(ENABLE_PCH "Compile with precompiled header" OFF)
option(ENABLE_UNITY "Compile using Unity Builds" ON)

add_library(ProjectSettings INTERFACE)
target_compile_features(ProjectSettings INTERFACE cxx_std_20)

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

# Define if it's the distribution config for main file
if ("${CMAKE_PRESET_NAME}" STREQUAL "x64-Distribution" OR "${CMAKE_PRESET_NAME}" STREQUAL "WSL-Distribution")
    add_compile_definitions(DISTRIBUTION)
endif ()

# Add external dependencies
add_subdirectory(external)

# Add unit testing target executable
if (COMPILE_TESTS)
    add_subdirectory(tests)
endif ()

# Add engine library
add_subdirectory(engine)

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.rc")

# Add executable with settings based on configuration used
if ("${CMAKE_PRESET_NAME}" STREQUAL "x64-Distribution")
    add_executable(CustomTech WIN32 "engine/main.cpp" ${APP_ICON_RESOURCE_WINDOWS})
elseif ("${CMAKE_PRESET_NAME}" STREQUAL "WSL-Distribution")
    add_executable(CustomTech "engine/main.cpp")
    set_target_properties(CustomTech
            PROPERTIES
            LINK_FLAGS "-mwindows"
    )
else ()
    add_executable(CustomTech "engine/main.cpp" ${APP_ICON_RESOURCE_WINDOWS})
endif ()

set_target_properties(CustomTech PROPERTIES OUTPUT_NAME "Blightspire")

target_link_libraries(CustomTech
        PRIVATE ProjectSettings
        PRIVATE Engine
)

### SHADERS COMPILE STEP ###
if (COMPILE_SHADERS)
    message(STATUS "### Shaders will be compiled on build")
    add_subdirectory(shaders)
    add_dependencies(CustomTech Shaders)
endif ()
