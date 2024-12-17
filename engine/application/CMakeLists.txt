add_library(Application)
module_default_init(Application)

target_link_libraries(Application
        PUBLIC Core
        PUBLIC Steam
        PUBLIC VulkanAPI #TODO: Might be able to remove this dependency
        PRIVATE ImGui #TODO: Only necessary to forward input to imgui
        PRIVATE SDL3::SDL3-static
        PRIVATE Steamworks
)