add_library(Core)
module_default_init(Core)

# Linked libraries or other modules (use PUBLIC for propagating includes)
target_link_libraries(Core
        PUBLIC Utility
        PRIVATE Scripting)