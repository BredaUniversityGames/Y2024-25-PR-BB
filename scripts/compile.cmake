# toolchain.cmake

set(CMAKE_CXX_FLAGS "-fexceptions -frtti -Wall -fuse-ld=lld")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS "-static")
