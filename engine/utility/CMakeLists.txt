add_library(Utility)
module_default_init(Utility)

target_link_libraries(Utility
        PUBLIC magic_enum::magic_enum
        PUBLIC Tracy::TracyClient
        PUBLIC spdlog::spdlog
        PUBLIC glm::glm
)