
## TODO: the external library should not exist after module transition
## Ideally, only add_subdirectory or fetch content calls will be here
## and each module will link to their required libraries

# ImGUI and ImPlot

file(GLOB imgui_sources CONFIGURE_DEPENDS "imgui/*.cpp")
file(GLOB imgui_headers CONFIGURE_DEPENDS "imgui/*.h")

set(imgui_files ${imgui_sources} ${imgui_headers}
        imgui/misc/cpp/imgui_stdlib.cpp
        imgui/misc/freetype/imgui_freetype.cpp
        imgui/backends/imgui_impl_vulkan.cpp
        imgui/backends/imgui_impl_sdl3.cpp
)

file(GLOB implot_sources CONFIGURE_DEPENDS "implot/*.cpp")
file(GLOB implot_headers CONFIGURE_DEPENDS "implot/*.h")
set(implot_files ${implot_sources} ${implot_headers})

add_library(ImGui ${imgui_files} ${implot_files})

target_include_directories(ImGui
        PRIVATE freetype/include
        PRIVATE Vulkan-Headers/include
        PRIVATE SDL/include
        PUBLIC imgui
        PUBLIC imgui/backends
        PUBLIC implot
)

# Magic Enum

add_subdirectory(magic_enum)

# SpdLog

add_subdirectory(spdlog)

# Vulkan

#TODO: Check if you can remove Vulkan-Headers from repo
find_package(Vulkan REQUIRED)

add_library(VulkanAPI INTERFACE)
target_link_libraries(VulkanAPI INTERFACE ${Vulkan_LIBRARY})
target_include_directories(VulkanAPI INTERFACE Vulkan-Headers/include)

# Vulkan Memory allocator

add_subdirectory(VulkanMemoryAllocator)

# FastGLTF

add_subdirectory(fastgltf)

# Tracy Profiler

set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)
add_subdirectory(tracy)

# EnTT

add_subdirectory(entt)

# GLM

add_subdirectory(glm)

# SDL

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(SDL)

# Main External Library

add_library(External INTERFACE)

if (WIN32)
    target_compile_options(External INTERFACE -DVK_USE_PLATFORM_WIN32_KHR)
elseif (LINUX)
    target_compile_options(External INTERFACE -DVK_USE_PLATFORM_XLIB_KHR)
endif ()

target_link_libraries(External
        INTERFACE ImGui
        INTERFACE SDL3::SDL3-static
        INTERFACE magic_enum::magic_enum
        INTERFACE VulkanMemoryAllocator
        INTERFACE fastgltf::fastgltf
        INTERFACE VulkanAPI
)

target_include_directories(External
        INTERFACE external/SDL/include
        INTERFACE stb
        INTERFACE entt/src/entt
)

