add_library(Scripting)
module_default_init(Scripting)

target_link_libraries(Scripting
        PRIVATE WrenLanguage
        PUBLIC Core
)