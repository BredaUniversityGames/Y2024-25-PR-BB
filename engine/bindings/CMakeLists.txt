add_library(Bindings)
module_default_init(Bindings)

target_link_libraries(Bindings
        PUBLIC Scripting
        PRIVATE Utility
        PRIVATE ECS
)

target_include_directories(Renderer
        PUBLIC "${PROJECT_SOURCE_DIR}/external/stb"
)
