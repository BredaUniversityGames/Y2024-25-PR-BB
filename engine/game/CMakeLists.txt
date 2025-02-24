add_library(Game)
module_default_init(Game)

target_link_libraries(Game
        PUBLIC Core
        PUBLIC ECS
        PUBLIC UserInterface
        PUBLIC Renderer
        PUBLIC ModelLoading
)
