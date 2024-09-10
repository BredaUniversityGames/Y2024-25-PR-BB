### PROJECT ###

cmake_minimum_required(VERSION 3.22.1)
project(Project)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### COMPILER SETTINGS ###

add_library(Compiler INTERFACE)

# USING C++ 20
target_compile_features(Compiler INTERFACE cxx_std_20)

# NEEDS TO BE CONFIGURED FOR BOTH GCC AND MSVC
target_compile_options(Compiler INTERFACE 
    "-W4"
)

### MAIN EXECUTABLE ###

# SOURCE FILES
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "engine/source/*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "engine/include/*.hpp")

# TEST FILES (only for test builds)
# file(GLOB_RECURSE tests CONFIGURE_DEPENDS "engine/tests/*.cpp")

add_executable(Engine ${sources} ${headers})
target_include_directories(Engine PRIVATE include)
target_link_libraries(Engine PRIVATE Compiler)

### EXTERNAL ###

include_directories(external)

add_subdirectory(external/magic_enum)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(external/SDL)

find_package(Vulkan)

if(WIN32)
    add_definitions(-DWINDOWS)
    if(NOT Vulkan_FOUND)
        find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
        if(Vulkan_LIBRARY)

            set(Vulkan_FOUND ON)
            message("WIN32: Using bundled Vulkan library version")
        endif()
    endif()
elseif(LINUX)
    add_definitions(-DLINUX) 
    if(NOT Vulkan_FOUND)
        find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
        if(Vulkan_LIBRARY)
            set(Vulkan_FOUND ON)
            message("LINUX: Using bundled Vulkan library version")
        endif()
    else()
        include_directories(${Vulkan_INCLUDE_DIRS})
    endif()


endif()

if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
else()
    message(STATUS ${Vulkan_LIBRARY})
endif()

# Link Vulkan to executable
link_libraries(${Vulkan_LIBRARY})

# Add and link glfw on Windows
if(WIN32)
    add_subdirectory(external/glfw)
    link_libraries(glfw)
endif()


file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/engine/src/*.cpp)

message(STATUS ${SOURCE_FILES})

list(REMOVE_DUPLICATES SOURCE_FILES)


target_link_libraries(Engine PRIVATE SDL3::SDL3-static)
target_include_directories(Engine PRIVATE external/SDL/include PRIVATE engine/include)

target_include_directories(Engine SYSTEM PRIVATE external/glm)
add_subdirectory(external/glm SYSTEM)

target_link_libraries(Engine PRIVATE magic_enum::magic_enum)
target_include_directories(Engine PUBLIC include)
target_compile_options(Engine PRIVATE -fexceptions -frtti $<$<CONFIG:Debug>:-O0 -g> $<$<CONFIG:Release>:-O3>)

if(WIN32)
	target_compile_options(Engine PRIVATE -DVK_USE_PLATFORM_WIN32_KHR)
elseif(LINUX)
    target_compile_options(Engine PRIVATE -DVK_USE_PLATFORM_XLIB_KHR)
endif()

target_compile_options(Engine PUBLIC -DNOMINMAX)

target_link_options(Engine PRIVATE
        $<$<CONFIG:Release>:-s>
)

# FREE TYPE

set(FREETYPE_DIR external/freetype/freetype2)

add_subdirectory(${FREETYPE_DIR})

target_link_libraries(Engine PUBLIC ${FREETYPE_LIBRARY})
target_include_directories(Engine SYSTEM PUBLIC ${FREETYPE_DIR}/include)

# END FREETYPE

# IMGUI

set(IMGUI_DIR external/imgui)

set(IMGUI_SOURCE_FILES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/implot.cpp
        ${IMGUI_DIR}/implot_items.cpp
        ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
        ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCE_FILES})

target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_include_directories(imgui PUBLIC ${FREETYPE_DIR}/include)
target_include_directories(imgui PUBLIC external/SDL/include)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

target_link_libraries(Engine PRIVATE imgui)
target_link_libraries(Engine PRIVATE ${XINPUT_LIBRARY})
target_include_directories(Engine PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends)

# END IMGUI


include_directories("external/stb")

add_subdirectory(external/spdlog)
target_link_libraries(Engine PRIVATE spdlog::spdlog)

add_subdirectory(external/fastgltf)
target_link_libraries(Engine PRIVATE fastgltf::fastgltf)

add_subdirectory(external/VulkanMemoryAllocator)
target_link_libraries(Engine PRIVATE VulkanMemoryAllocator)

# BUILD TYPE SETTINGS

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(Engine PRIVATE DEBUG_MODE)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(Engine PRIVATE NDEBUG)
endif()

# END BUILD TYPE SETTINGS

