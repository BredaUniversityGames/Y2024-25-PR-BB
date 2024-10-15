include(FetchContent)

# Macro for overriding default values for options before adding a library
macro(override_option option_name option_value)
    set(${option_name} ${option_value} CACHE BOOL "" FORCE)
endmacro()

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
        GIT_TAG v0.8.0
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

override_option(SKIP_PERFORMANCE_COMPARISON ON)
override_option(BUILD_SANDBOX OFF)
override_option(BUILD_TESTS OFF)
override_option(BUILD_DOC OFF)

FetchContent_MakeAvailable(cereal)

# Tracy Profiler

FetchContent_Declare(
        Tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.11.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

override_option(TRACY_ENABLE ON)
override_option(TRACY_ON_DEMAND ON)
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

override_option(SDL_SHARED OFF)
override_option(SDL_STATIC ON)
FetchContent_MakeAvailable(SDL)

# GLM

FetchContent_Declare(
        GLM
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(GLM)

# Jolt

set(INTERPROCEDURAL_OPTIMIZATION ON)
# Select X86 processor features to use, by default the library compiles with AVX2, if everything is off it will be SSE2 compatible.
set(USE_SSE4_1 ON)
set(USE_SSE4_2 ON)
set(USE_AVX ON)
set(USE_AVX2 ON)
set(USE_AVX512 OFF)
set(USE_LZCNT ON)
set(USE_TZCNT ON)
set(USE_F16C ON)
set(USE_FMADD ON)
set(CPP_RTTI_ENABLED ON)

# Include Jolt
FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
        GIT_TAG "0a7d2501b33e1251101392d2be403c551ce5f963"
        SOURCE_SUBDIR "Build"
)
FetchContent_MakeAvailable(JoltPhysics)
SET_INTERPROCEDURAL_OPTIMIZATION()

# Set compiler options for Jolt
target_compile_options(Jolt PRIVATE
        -w          # Suppress all warnings
        -Wno-error  # Do not treat warnings as errors
)

# GoogleTest

FetchContent_Declare(
        GoogleTest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
override_option(gtest_force_shared_crt ON)
FetchContent_MakeAvailable(GoogleTest)

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

add_library(ImGui ${imgui_files} ${implot_files})
target_compile_definitions(ImGui PRIVATE SDL_ENABLE_OLD_NAMES)

target_link_libraries(ImGui
        PRIVATE SDL3::SDL3-static
)

target_include_directories(ImGui
        PRIVATE freetype/include
        PRIVATE Vulkan-Headers/include
        PUBLIC imgui
        PUBLIC imgui/backends
        PUBLIC implot
)

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
        INTERFACE spdlog::spdlog
        INTERFACE glm::glm
        INTERFACE EnTT::EnTT
        INTERFACE VulkanMemoryAllocator
        INTERFACE fastgltf::fastgltf
        INTERFACE VulkanAPI
        INTERFACE Jolt
)

target_include_directories(External
        INTERFACE external/SDL/include
        INTERFACE stb
        INTERFACE ${cereal_SOURCE_DIR}/include
)

