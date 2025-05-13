add_library(Inspector)
module_default_init(Inspector)

target_link_libraries(Inspector
        PUBLIC Renderer
        PUBLIC ImGui
        PRIVATE Particles
        PUBLIC Game
        PUBLIC Settings
        PRIVATE Physics
        PRIVATE Pathfinding
)

target_include_directories(Inspector
        PUBLIC "${PROJECT_SOURCE_DIR}/external/visit_struct/include"
)
