add_library(Scripting
        public/callback.hpp
        public/callback.hpp
        public/callback.hpp)
module_default_init(Scripting)

target_link_libraries(Scripting
        PUBLIC WrenBind17
        PUBLIC Wren
        PUBLIC Core
        PRIVATE Bindings
)