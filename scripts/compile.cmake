# toolchain.cmake

# set(CMAKE_C_COMPILER "clang")
# set(CMAKE_CXX_COMPILER "clang++")

set(CMAKE_CXX_FLAGS "-fexceptions -frtti")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
