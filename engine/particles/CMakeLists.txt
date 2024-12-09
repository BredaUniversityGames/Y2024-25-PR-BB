add_library(Particles)
module_default_init(Particles)

target_link_libraries(Particles
        PUBLIC Core
        PUBLIC ECS
        PRIVATE Renderer
        PRIVATE Physics
)