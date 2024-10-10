add_library(Utility)

# Specifies all compilation flags (DO NOT REMOVE)
target_link_libraries(Utility PRIVATE ProjectSettings)

# Get all files in local module tree (headers and sources)

file(GLOB_RECURSE public_files CONFIGURE_DEPENDS "public/*.hpp")
file(GLOB_RECURSE private_files CONFIGURE_DEPENDS "private/*.hpp" "private/*.cpp")
target_sources(Utility PUBLIC ${public_files} PRIVATE ${private_files})

# Public include directories (just "public")
target_include_directories(Utility PUBLIC "public")

# Private include directories (just "private")
target_include_directories(Utility PRIVATE "private")

target_link_libraries(Utility
        PUBLIC Tracy::TracyClient
        PUBLIC spdlog::spdlog
        PUBLIC glm::glm
)