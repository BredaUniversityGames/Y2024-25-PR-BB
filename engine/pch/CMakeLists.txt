add_library(PCH)
target_sources(PCH PUBLIC "pch.hpp" PRIVATE "pch.cpp")
target_precompile_headers(PCH PRIVATE "pch.hpp")

# Inherit same compiler flags as third party

target_link_libraries(PCH
        PRIVATE ProjectSettings
        PUBLIC External
        PUBLIC Utility
)
