add_library(Physics)
module_default_init(Physics)

target_link_libraries(Physics
    PUBLIC Core
    PUBLIC Application
    PUBLIC OldEngine
)
