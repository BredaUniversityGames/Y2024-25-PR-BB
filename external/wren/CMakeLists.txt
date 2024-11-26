add_library(WrenLanguage)

file(GLOB_RECURSE wren_sources CONFIGURE_DEPENDS "vm/*.c" "vm/*.h")
file(GLOB_RECURSE wren_optional_sources CONFIGURE_DEPENDS "optional/*.c" "optional/*.h")

target_compile_definitions(WrenLanguage
        PRIVATE WREN_OPT_META=1
        PRIVATE WREN_OPT_RANDOM=1)

target_include_directories(WrenLanguage
        PRIVATE "vm"
        PRIVATE "optional"
        PUBLIC "include")

target_sources(WrenLanguage
        PRIVATE ${wren_sources}
        PRIVATE ${wren_optional_sources}
        PUBLIC "include/wren.h"
        PUBLIC "include/wren.hpp")