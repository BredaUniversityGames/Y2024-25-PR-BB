add_library(PCH)
target_sources(PCH PUBLIC "pch.hpp" PRIVATE "pch.cpp")
target_precompile_headers(PCH PRIVATE "pch.hpp")

# Precompiled headers require that all TU use the same compilation flags
# Linking to some externals here guarantees that the precompiled header has all of the compile flags available
# And that every cpp file in the project will inherit those flags
target_link_libraries(PCH
        PRIVATE ProjectSettings
        PUBLIC Jolt
        PUBLIC Utility
)
