add_library(Pathfinding
        private/pathfinding_module.cpp)
module_default_init(Pathfinding)

target_link_libraries(Pathfinding
        PUBLIC Core
        PUBLIC Application
        PUBLIC OldEngine
        PUBLIC Physics
)