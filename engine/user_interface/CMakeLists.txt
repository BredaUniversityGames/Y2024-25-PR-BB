add_library(UserInterface)
module_default_init(UserInterface)

# Linked libraries or other modules (use PUBLIC for propagating includes)
target_link_libraries(UserInterface
        PUBLIC Engine
        PUBLIC Utility
        PUBLIC Core
        PRIVATE Application
        PUBLIC freetype)