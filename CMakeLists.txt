### PROJECT ###
cmake_minimum_required(VERSION 3.22.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(Project)

### COMPILATION SETTINGS ###

option(WARNINGS_AS_ERRORS "Enable warnings as errors" OFF)

add_library(ProjectSettings INTERFACE)
target_compile_features(ProjectSettings INTERFACE cxx_std_20)

if (WARNINGS_AS_ERRORS)
    message(STATUS "### Warnings are enabled as Errors")
    target_compile_options(ProjectSettings INTERFACE -Werror)
endif ()

#TODO: Use reused precompiled headers
#option(ENABLE_PCH "Compile with precompiled header" ON)
#
#if (ENABLE_PCH)
#    target_precompile_headers(ProjectSettings INTERFACE "engine/pch.hpp")
#endif ()

# Add external dependencies
add_subdirectory(external)

# Add engine library
add_subdirectory(engine)

# Add unit testing target executable
option(ENABLE_TESTS "Compile with Unit Tests" ON)
if (ENABLE_TESTS)
    message(STATUS "### Unit Tests are enabled")
    
    add_subdirectory(external/googletest)

    add_executable(UnitTests)

    file(GLOB_RECURSE tests CONFIGURE_DEPENDS "tests/*.cpp")
    target_sources(UnitTests PRIVATE ${tests})

    target_link_libraries(UnitTests
            PRIVATE ProjectSettings
            PRIVATE Engine
            PRIVATE GTest::gtest_main
    )

endif ()

# Add game executable (currently just main.cpp)
add_executable(Game "engine/main.cpp")
target_link_libraries(Game
        PRIVATE ProjectSettings
        PRIVATE Engine
)

### SHADERS COMPILE STEP ###

option(COMPILE_SHADERS "Compile all GLSL shaders as part of build step" ON)
if (COMPILE_SHADERS)
    message(STATUS "### Shaders will be compiled on build")
    add_subdirectory(shaders)
    add_dependencies(Game Shaders)
endif ()

