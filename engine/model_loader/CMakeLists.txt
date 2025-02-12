add_library(ModelLoader)
module_default_init(ModelLoader)

target_link_libraries(ModelLoader
        PUBLIC Core
        PUBLIC Renderer

        PUBLIC fastgltf::fastgltf
)

target_include_directories(Renderer
        PUBLIC "${PROJECT_SOURCE_DIR}/external/stb"
)
