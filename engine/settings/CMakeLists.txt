add_library(Settings)
module_default_init(Settings)

target_link_libraries(Settings
        PRIVATE cereal
)

target_include_directories(Settings
        PUBLIC "${PROJECT_SOURCE_DIR}/external/visit_struct/include"
)
