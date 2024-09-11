### PROJECT ###

cmake_minimum_required(VERSION 3.22.1)
project(Project)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# WARN, these should be set on the preset or launch config than here
set(CMAKE_CXX_FLAGS_INIT "/W4 /EHs /GR")


### MAIN EXECUTABLE ###


# SOURCE FILES
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "engine/source/*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "engine/include/*.hpp")


# TEST FILES (only for test builds)
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "engine/tests/*.cpp")


# EXECUTABLE TARGET
add_executable(Engine ${sources} ${headers})


add_subdirectory(external)
target_link_libraries(Engine PRIVATE External)


target_include_directories(Engine PRIVATE engine/include external)
target_compile_features(Engine PRIVATE cxx_std_20)

