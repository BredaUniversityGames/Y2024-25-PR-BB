add_library(Renderer)
module_default_init(Renderer)

target_link_libraries(Renderer
        PUBLIC Core
        PUBLIC Application
        PUBLIC ECS
)

target_include_directories(Renderer
        PUBLIC stb
)
