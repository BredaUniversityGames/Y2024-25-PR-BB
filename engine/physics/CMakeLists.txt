add_library(Physics)
module_default_init(Physics)

target_link_libraries(Physics
        PUBLIC Core
        PUBLIC Application
        PUBLIC Jolt
        PUBLIC ECS
        PUBLIC ModelLoading
        PRIVATE Renderer
        PRIVATE ImGui
)
