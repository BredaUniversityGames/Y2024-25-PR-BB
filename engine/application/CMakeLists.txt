add_library(Application)
module_default_init(Application)

target_link_libraries(Application
        PUBLIC Core
        PUBLIC VulkanAPI #TODO: Might be able to remove this dependency
        PRIVATE ImGui #TODO: Only necessary Process Events in ImGui
        PRIVATE SDL3::SDL3-static
)