add_library(ECS)
module_default_init(ECS)

target_link_libraries(ECS
        PUBLIC Core
        PUBLIC EnTT::EnTT
        PUBLIC cereal
        PUBLIC ImGui
)
