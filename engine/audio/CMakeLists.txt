add_library(Audio)
module_default_init(Audio)

target_link_libraries(Audio
        PUBLIC Core
        PUBLIC ECS
        PRIVATE FMOD
)
