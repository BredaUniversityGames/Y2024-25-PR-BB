add_library(Settings)
module_default_init(Settings)

target_link_libraries(Settings
        PRIVATE cereal
)
