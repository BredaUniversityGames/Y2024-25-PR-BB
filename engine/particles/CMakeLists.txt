add_library(Particles)
module_default_init(Particles)

target_link_libraries(Particles
        PUBLIC Utility
        PUBLIC Core
        PUBLIC Renderer
        PUBLIC ECS
)