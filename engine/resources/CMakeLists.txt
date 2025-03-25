add_library(Resources)
module_default_init(Resources)

target_link_libraries(Resources
        PUBLIC Core
        PUBLIC Renderer
        PUBLIC Thread

        PRIVATE fastgltf::fastgltf
)

target_include_directories(Resources
        PRIVATE "${PROJECT_SOURCE_DIR}/external/stb"
)

source_exclude_unity("private/lib/stbi_impl.cpp")