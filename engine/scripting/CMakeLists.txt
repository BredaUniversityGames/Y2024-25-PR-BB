add_library(Scripting)
module_default_init(Scripting)

target_link_libraries(Scripting
        PUBLIC WrenBind17
        PUBLIC Wren
        PUBLIC Core
)