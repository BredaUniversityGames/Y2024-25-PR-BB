add_library(Settings)
module_default_init(Settings)

target_link_libraries(Settings
        PUBLIC Utility

        PRIVATE cereal
        PUBLIC glm::glm
)

target_include_directories(Settings
        PUBLIC "${PROJECT_SOURCE_DIR}/external/visit_struct/include"
)
