add_library(Core)
module_default_init(Core)

# Linked libraries or other modules (use PUBLIC for propagating includes)
target_link_libraries(Core PRIVATE)

# TODO: Create and link to dedicated util module
target_include_directories(Core PUBLIC "../util")