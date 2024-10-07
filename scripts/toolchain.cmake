
# Use GCC to compile the project (by default, CLion uses GCC)
# you will have to set in your cmake command

# this file only specifies global compilation flags

set(CMAKE_CXX_FLAGS "-fexceptions -frtti -fmax-errors=0")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
