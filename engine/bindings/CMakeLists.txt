add_library(Bindings)
module_default_init(Bindings)

target_link_libraries(Bindings
        PUBLIC Scripting
        PRIVATE Application
        PRIVATE Utility
        PRIVATE ECS
        PRIVATE Audio
        PRIVATE Renderer
        PRIVATE Particles
        PRIVATE GameAnalytics
        PRIVATE magic_enum::magic_enum
)

target_include_directories(Renderer
        PUBLIC "${PROJECT_SOURCE_DIR}/external/stb"
)
