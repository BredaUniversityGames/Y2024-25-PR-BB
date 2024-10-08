### PROJECT ###
cmake_minimum_required(VERSION 3.22.1)
project(Project)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Check for build type and define macros
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_BUILD)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(RELEASE_BUILD)
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_definitions(RELWITHDEBINFO_BUILD)
elseif (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    add_compile_definitions(MINSIZEREL_BUILD)
endif ()

### MAIN EXECUTABLE ###

# SOURCE FILES
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "engine/source/*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "engine/include/*.hpp")

# EXECUTABLE TARGET

add_executable(Engine ${sources} ${headers})

### SHADERS ###

option(COMPILE_SHADERS "Compile all GLSL shaders as part of build step" ON)

if (COMPILE_SHADERS)
    message(STATUS "### Shaders will be compiled on build")
    add_subdirectory(shaders)
    add_dependencies(Engine Shaders)
endif ()

option(WARNINGS_AS_ERRORS OFF)

if (WARNINGS_AS_ERRORS)
    message(STATUS "### Warnings are enabled as Errors")
    target_compile_options(Engine PRIVATE -Werror)
endif ()

option(ENABLE_PCH "Compile with precompiled header" ON)

if (ENABLE_PCH)
    target_precompile_headers(Engine PRIVATE [["pch.hpp"]])
endif ()

option(ENABLE_TESTS "Compile with Unit Tests" ON)

if (ENABLE_TESTS)
    message(STATUS "### Tests are Enabled (GoogleTest)")

    add_subdirectory(external/googletest)
    enable_testing()

    # Glob all test files
    file(GLOB_RECURSE tests CONFIGURE_DEPENDS "tests/*.cpp")

    target_link_libraries(Engine PRIVATE gtest)
    target_sources(Engine PRIVATE ${tests})

    # Include GoogleTest headers
    target_include_directories(Engine
            PUBLIC external/googletest/googletest/include
            PUBLIC external/googletest/googlemock/include
    )

    target_compile_definitions(Engine PRIVATE TESTS_ENABLED)

endif ()

add_subdirectory(external)
add_subdirectory(engine/core)
target_link_libraries(Engine PRIVATE External PRIVATE Core)

target_include_directories(Engine PRIVATE engine/include external)
target_compile_features(Engine PRIVATE cxx_std_20)

