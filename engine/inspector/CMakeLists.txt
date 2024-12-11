add_library(Inspector)
module_default_init(Inspector)

target_link_libraries(Inspector
        PUBLIC Renderer
        PUBLIC ImGui
)