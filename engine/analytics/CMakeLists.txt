add_library(Analytics)
module_default_init(Analytics)

target_link_libraries(Analytics
        PUBLIC Engine
        PUBLIC Core
        PUBLIC Utility
        PRIVATE GameAnalytics
)
