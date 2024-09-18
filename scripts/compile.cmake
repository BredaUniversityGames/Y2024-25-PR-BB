# toolchain.cmake

# Target operating system name (Linux, Windows, etc.)
set(CMAKE_SYSTEM_NAME Windows)

# Specify the target architecture
set(CMAKE_SYSTEM_PROCESSOR x64)

set(CMAKE_CXX_FLAGS "-fexceptions -frtti -Wall -fuse-ld=lld")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
