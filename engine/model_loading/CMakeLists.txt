add_library(ModelLoading)
module_default_init(ModelLoading)

target_link_libraries(ModelLoading
        PUBLIC Core
        PUBLIC Renderer

        PUBLIC fastgltf::fastgltf
)

target_include_directories(ModelLoading
        PUBLIC "${PROJECT_SOURCE_DIR}/external/stb"
)
