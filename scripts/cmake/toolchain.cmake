# Compiler is specified in CMakePresets (we need clang for cpp-linter)

# set(CMAKE_C_COMPILER "gcc")
set(CMAKE_C_FLAGS "-std=c99 -fmax-errors=0")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O4 -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

# set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-std=c++20 -fexceptions -frtti -fmax-errors=0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# Use the default linker and archiver
# set(CMAKE_AR "llvm-ar")
# set(CMAKE_LINKER "lld")

if (WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif ()
