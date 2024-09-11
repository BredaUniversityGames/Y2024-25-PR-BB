### PROJECT ###


cmake_minimum_required(VERSION 3.22.1)
project(Project)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_INIT "/W4 /EHs /GR")


### MAIN EXECUTABLE ###

# SOURCE FILES
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "engine/source/*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "engine/include/*.hpp")

# TEST FILES (only for test builds)
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "engine/tests/*.cpp")

# EXECUTABLE TARGET
add_executable(Engine ${sources} ${headers})

target_include_directories(Engine PRIVATE engine/include external)
target_compile_features(Engine PRIVATE cxx_std_20)


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

target_link_libraries(Engine PRIVATE SDL3::SDL3-static)
target_include_directories(Engine PRIVATE external/SDL/include PRIVATE engine/include)

# GLM

target_include_directories(Engine SYSTEM PRIVATE external/glm)
target_link_libraries(Engine PRIVATE magic_enum::magic_enum)

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

set(FREETYPE_DIR external/freetype)
add_subdirectory(${FREETYPE_DIR})
target_link_libraries(Engine PUBLIC ${FREETYPE_LIBRARY})
target_include_directories(Engine SYSTEM PUBLIC ${FREETYPE_DIR}/include)

# END FREETYPE

# IMGUI

set(IMGUI_DIR external/imgui)
set(IMPLOT_DIR external/implot)

set(IMGUI_SOURCE_FILES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMPLOT_DIR}/implot.cpp
        ${IMPLOT_DIR}/implot_items.cpp
        ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
        ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCE_FILES})

target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_include_directories(imgui PUBLIC ${FREETYPE_DIR}/include)
target_include_directories(imgui PUBLIC external/Vulkan-Headers/include)
target_include_directories(imgui PUBLIC external/)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

target_link_libraries(Engine PRIVATE imgui)
target_link_libraries(Engine PRIVATE ${XINPUT_LIBRARY})
target_include_directories(Engine PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_include_directories(Engine PRIVATE external/Vulkan-Headers/include)
target_include_directories(Engine PRIVATE ${IMPLOT_DIR})

# END IMGUI


include_directories("external/stb")

add_subdirectory(external/spdlog)
target_link_libraries(Engine PRIVATE spdlog::spdlog)

add_subdirectory(external/fastgltf)
target_link_libraries(Engine PRIVATE fastgltf::fastgltf)

add_subdirectory(external/VulkanMemoryAllocator)
target_link_libraries(Engine PRIVATE VulkanMemoryAllocator)


# END BUILD TYPE SETTINGS

