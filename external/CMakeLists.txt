# Magic Enum

FetchContent_Declare(
        MagicEnum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
        GIT_TAG v0.9.6
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(MagicEnum)

# SpdLog

FetchContent_Declare(
        SpdLog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(SpdLog)

# Vulkan

find_package(Vulkan REQUIRED)
add_library(VulkanAPI INTERFACE)

target_link_libraries(VulkanAPI INTERFACE ${Vulkan_LIBRARY})
target_include_directories(VulkanAPI INTERFACE Vulkan-Headers/include)
target_compile_definitions(VulkanAPI INTERFACE VULKAN_HPP_NO_CONSTRUCTORS)

# Vulkan Memory allocator

FetchContent_Declare(
        VMA
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG v3.1.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(VMA)

# FastGLTF

FetchContent_Declare(
        FastGLTF
        GIT_REPOSITORY https://github.com/spnda/fastgltf.git
        GIT_TAG main
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(FastGLTF)

# Cereal

FetchContent_Declare(
        cereal
        GIT_REPOSITORY https://github.com/USCiLab/cereal.git
        GIT_TAG v1.3.2
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

# FMOD (no git to fetch content)

add_subdirectory(fmod)

# Cereal

option(BUILD_DOC "Build documentation" OFF)
option(BUILD_SANDBOX "Build sandbox examples" OFF)
option(SKIP_PERFORMANCE_COMPARISON "Skip building performance sandbox comparison (requires boost)" ON)

FetchContent_MakeAvailable(cereal)

# Wren Language

FetchContent_Declare(
        wren
        GIT_REPOSITORY https://github.com/santiberna/wrenbind17.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

set(WRENBIND17_BUILD_WREN ON)
FetchContent_MakeAvailable(wren)


# Tracy Profiler

FetchContent_Declare(
        Tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.11.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(TRACY_ENABLE OFF)
else ()
    set(TRACY_ENABLE ON)
endif ()

set(TRACY_ON_DEMAND ON)

FetchContent_MakeAvailable(Tracy)

# EnTT

FetchContent_Declare(
        EnTT
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.13.2
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(EnTT)

# SDL

FetchContent_Declare(
        SDL
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG preview-3.1.3
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

set(SDL_SHARED OFF)
set(SDL_STATIC ON)
FetchContent_MakeAvailable(SDL)

target_compile_options(SDL3-static PRIVATE -Wno-deprecated-declarations)

# GLM

FetchContent_Declare(
        GLM
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(GLM)

# Guarantees GLM follows the same perspective convention as Vulkan
target_compile_definitions(glm PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE PUBLIC GLM_ENABLE_EXPERIMENTAL)

# Jolt

FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
        GIT_TAG "0a7d2501b33e1251101392d2be403c551ce5f963"
        SOURCE_SUBDIR "Build"
)

set(INTERPROCEDURAL_OPTIMIZATION OFF)
set(CPP_RTTI_ENABLED ON)
set(ENABLE_ALL_WARNINGS OFF)

# TODO: These options enable vectorization optimizations
set(USE_SSE4_1 ON)
set(USE_SSE4_2 ON)
set(USE_AVX ON)
set(USE_AVX2 ON)
set(USE_AVX512 OFF)
set(USE_LZCNT ON)
set(USE_TZCNT ON)
set(USE_F16C ON)
set(USE_FMADD ON)

FetchContent_MakeAvailable(JoltPhysics)

if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(Jolt PUBLIC JPH_DEBUG_RENDERER)
endif ()

SET_INTERPROCEDURAL_OPTIMIZATION()

# Set compiler options for Jolt
target_compile_options(Jolt PRIVATE
        -w          # Suppress all warnings
        -Wno-error  # Do not treat warnings as errors
)

# SPIRV-Reflect

FetchContent_Declare(
        SPIRV-Reflect
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
        GIT_TAG vulkan-sdk-1.3.296.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
override_option(SPIRV_REFLECT_STATIC_LIB ON)
FetchContent_MakeAvailable(SPIRV-Reflect)

# ImGUI and ImPlot (cannot be used with FetchContent without forking)

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

add_library(ImGui ${imgui_files} ${implot_files} "imgui_entt_entity_editor/imgui_entt_entity_editor.hpp")
target_compile_definitions(ImGui PRIVATE SDL_ENABLE_OLD_NAMES)

target_link_libraries(ImGui
        PRIVATE SDL3::SDL3-static
        PRIVATE VulkanAPI
)

target_include_directories(ImGui
        PRIVATE freetype/include
        PUBLIC imgui
        PUBLIC imgui/backends
        PUBLIC implot
        PUBLIC imgui_entt_entity_editor
)

# Steamworks API

add_subdirectory(steamworks)

# Enable Unity for external libraries

if (ENABLE_UNITY)
    target_enable_unity(Jolt)
endif ()

# Main External Library

add_library(External INTERFACE)

if (WIN32)
    target_compile_options(External
            INTERFACE -DVK_USE_PLATFORM_WIN32_KHR
            INTERFACE -DNOMINMAX)
elseif (LINUX)
    target_compile_options(External
            INTERFACE -DVK_USE_PLATFORM_XLIB_KHR)
endif ()

target_link_libraries(External
        INTERFACE ImGui
        INTERFACE SDL3::SDL3-static
        INTERFACE magic_enum::magic_enum
        INTERFACE spdlog::spdlog
        INTERFACE glm::glm
        INTERFACE EnTT::EnTT
        INTERFACE VulkanMemoryAllocator
        INTERFACE fastgltf::fastgltf
        INTERFACE spirv-reflect-static
        INTERFACE VulkanAPI
        INTERFACE Jolt
        INTERFACE Steamworks
        INTERFACE FMOD
)

target_include_directories(External
        INTERFACE external/SDL/include
        INTERFACE stb
        INTERFACE ${cereal_SOURCE_DIR}/include
)