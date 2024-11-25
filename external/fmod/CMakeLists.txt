set(FMOD_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(FMOD_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_library(FMOD INTERFACE)

target_include_directories(FMOD INTERFACE ${FMOD_INCLUDE_DIR})

if (WIN32)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(FMOD_LIBRARIES
                "${FMOD_LIB_DIR}/Windows/Debug/fmodL.dll")
    else ()
        set(FMOD_LIBRARIES
                "${FMOD_LIB_DIR}/Windows/Release/fmod.dll")
    endif ()
elseif (UNIX)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(FMOD_LIBRARIES
                "${FMOD_LIB_DIR}/Linux/Debug/libfmodL.so.14.4")
    else ()
        set(FMOD_LIBRARIES
                "${FMOD_LIB_DIR}/Linux/Release/libfmod.so.14.4")
    endif ()
endif ()

target_link_libraries(FMOD INTERFACE ${FMOD_LIBRARIES})