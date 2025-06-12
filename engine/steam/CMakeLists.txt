add_library(Steam)
module_default_init(Steam)

target_link_libraries(Steam
        PUBLIC Steamworks
        PUBLIC Core
)