add_library(PCH)
target_sources(PCH PUBLIC "pch.hpp" PRIVATE "pch.cpp")
target_precompile_headers(PCH PRIVATE "pch.hpp")

# Precompiled headers require that all TU use the same compilation flags
# Linking to External here guarantees that the precompiled header has all of the external compile flags available
# And that every cpp file in the project will inherit those flags
target_link_libraries(PCH
        PRIVATE ProjectSettings
        PUBLIC External
        PUBLIC Utility
)
