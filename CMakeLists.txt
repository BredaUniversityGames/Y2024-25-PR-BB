### PROJECT ###
cmake_minimum_required(VERSION 3.22.1)
project(Project)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### MAIN EXECUTABLE ###

# SOURCE FILES
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "engine/source/*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "engine/include/*.hpp")

# EXECUTABLE TARGET
add_executable(Engine ${sources} ${headers})

option(ENABLE_TESTS "Compile with Unit Tests" ON)

if (ENABLE_TESTS)

	message("-- Tests are Enabled (GoogleTest)")

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

endif()

add_subdirectory(external)
target_link_libraries(Engine PRIVATE External)

target_include_directories(Engine PRIVATE engine/include external)
target_compile_features(Engine PRIVATE cxx_std_20)

