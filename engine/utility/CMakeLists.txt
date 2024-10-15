add_library(Utility)
module_default_init(Utility)

target_link_libraries(Utility
        PUBLIC Tracy::TracyClient
        PUBLIC spdlog::spdlog
        PUBLIC glm::glm
)