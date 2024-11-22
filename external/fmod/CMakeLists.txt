set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--enable-auto-import")
set(CMAKE_VERBOSE_MAKEFILE ON)

set(FMOD_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(FMOD_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(BUILD_CONFIG_POSTFIX $<$<CONFIG:Debug>:L>
        $<$<CONFIG:RelWithDebInfo>:L>
)

add_library(FMOD)

file(GLOB fmod_sources CONFIGURE_DEPENDS "include/*.cpp")
file(GLOB fmod_headers CONFIGURE_DEPENDS "include/*.h")

target_sources(FMOD PRIVATE ${fmod_sources})

target_include_directories(FMOD PUBLIC ${FMOD_INCLUDE_DIR})

if (WIN32)
    set(FMOD_LIBRARIES
            "${FMOD_LIB_DIR}/x64/fmod.dll")

endif ()

target_link_libraries(FMOD PRIVATE ${FMOD_LIBRARIES})


#set(PLATFORM $<$<BOOL:${WIN32}>:x64>)
#
#if (PLATFORM MATCHES "")
#    message(FATAL_ERROR "Neither WIN32 nor UNIX were defined")
#endif ()
#
#include_directories(${FMOD_INCLUDE_DIR})
#
#set(FMOD_LIB_DIR "lib/x64/fmod${BUILD_CONFIG_POSTFIX}_vc.lib")


