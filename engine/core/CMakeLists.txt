# Declare module library
add_library(Core)
message(STATUS "### - Core Module")

# Specifies all compilation flags (DO NOT REMOVE)
target_link_libraries(Core PRIVATE ProjectSettings)

# Get all files in local module tree (headers and sources)

file(GLOB_RECURSE public_files CONFIGURE_DEPENDS "public/*.hpp")
file(GLOB_RECURSE private_files CONFIGURE_DEPENDS "private/*.hpp" "private/*.cpp")
target_sources(Core PUBLIC ${public_files} PRIVATE ${private_files})

# Public include directories (just "public")
target_include_directories(Core PUBLIC "public")

# Private include directories (just "private")
target_include_directories(Core PRIVATE "private")

# Linked libraries or other modules (use PUBLIC for propagating includes)
target_link_libraries(Core PRIVATE)

# TODO: Create and link to dedicated util module
target_include_directories(Core PUBLIC "../util")