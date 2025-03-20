add_library(ModelLoading)
module_default_init(ModelLoading)

target_link_libraries(ModelLoading
        PUBLIC Core
        PUBLIC Renderer
        PUBLIC Thread

        PRIVATE fastgltf::fastgltf
)

target_include_directories(ModelLoading
        PRIVATE "${PROJECT_SOURCE_DIR}/external/stb"
)

source_exclude_unity("private/lib/stbi_impl.cpp")