add_library(Pathfinding)
module_default_init(Pathfinding)

target_link_libraries(Pathfinding
        PUBLIC Core
        PUBLIC Application
        PUBLIC Physics
        PUBLIC Renderer
        PUBLIC ModelLoading
)
