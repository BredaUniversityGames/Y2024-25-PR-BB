add_library(Renderer)
module_default_init(Renderer)

target_link_libraries(Renderer
        PUBLIC Core
        PUBLIC Application
        PUBLIC Particles
        PUBLIC ECS
        PUBLIC UserInterface
        PUBLIC Settings

        PUBLIC VulkanAPI
        PUBLIC VulkanMemoryAllocator
        PUBLIC spirv-reflect-static
)

source_exclude_unity("private/lib/stbi_impl.cpp")
source_exclude_unity("private/lib/vma_impl.cpp")