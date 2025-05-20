add_library(Utility)
module_default_init(Utility)

target_link_libraries(Utility
        PUBLIC magic_enum::magic_enum
        PUBLIC Tracy::TracyClient
        PUBLIC spdlog::spdlog
        PUBLIC glm::glm
        PUBLIC cereal::cereal
)

target_include_directories(Utility
        PUBLIC "${PROJECT_SOURCE_DIR}/external/visit_struct/include"
)
