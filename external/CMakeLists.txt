
# ImGUI

file(GLOB imgui_sources CONFIGURE_DEPENDS "imgui/*.cpp")
file(GLOB imgui_headers CONFIGURE_DEPENDS "imgui/*.h")

set(imgui_files ${imgui_sources} ${imgui_headers}
	imgui/misc/cpp/imgui_stdlib.cpp
    imgui/misc/freetype/imgui_freetype.cpp
    imgui/backends/imgui_impl_vulkan.cpp
    imgui/backends/imgui_impl_sdl3.cpp
)

# ImPlot

file(GLOB implot_sources CONFIGURE_DEPENDS "implot/*.cpp")
file(GLOB implot_headers CONFIGURE_DEPENDS "implot/*.h")
set(implot_files ${implot_sources} ${implot_headers})

# SDL

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(SDL)

# Magic Enum

add_subdirectory(magic_enum)

# SpdLog

add_subdirectory(spdlog)

# Vulkan

find_package(Vulkan)
add_subdirectory(VulkanMemoryAllocator)

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

# FastGLTF

add_subdirectory(fastgltf)

# Main External Library

add_library(External ${imgui_files} ${implot_files})

if(WIN32)
	target_compile_options(External PUBLIC -DVK_USE_PLATFORM_WIN32_KHR)
elseif(LINUX)
    target_compile_options(External PUBLIC -DVK_USE_PLATFORM_XLIB_KHR)
endif()

target_link_libraries(External
    PUBLIC SDL3::SDL3-static
    PUBLIC magic_enum::magic_enum
    PUBLIC spdlog::spdlog
    PUBLIC VulkanMemoryAllocator
    PUBLIC fastgltf::fastgltf
    PUBLIC ${Vulkan_LIBRARY}
)

target_include_directories(External 
    PUBLIC external/SDL/include 
    PUBLIC Vulkan-Headers/include
    PUBLIC imgui
    PUBLIC imgui/backends
    PUBLIC implot
    PUBLIC glm 
    PUBLIC stb
    PRIVATE freetype/include
)