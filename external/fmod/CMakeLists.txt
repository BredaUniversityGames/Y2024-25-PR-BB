set(FMOD_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(FMOD_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(BUILD_CONFIG_POSTFIX $<$<CONFIG:Debug>:L>
        $<$<CONFIG:RelWithDebInfo>:L>
)

file(GLOB fmod_sources CONFIGURE_DEPENDS "include/*.cpp")
file(GLOB fmod_headers CONFIGURE_DEPENDS "include/*.h")

if (WIN32)
    set(FMOD_LIBRARIES
            "${FMOD_LIB_DIR}/x64/fmod_vc.lib"
            "${FMOD_LIB_DIR}/x64/fmodL_vc.lib")
endif ()

add_library(FMOD ${fmod_sources} "include/fmod.hpp" ${fmod_headers})

target_include_directories(FMOD PUBLIC ${FMOD_INCLUDE_DIR})

target_link_libraries(FMOD PUBLIC ${FMOD_LIBRARIES})


#set(PLATFORM $<$<BOOL:${WIN32}>:x64>)
#
#if (PLATFORM MATCHES "")
#    message(FATAL_ERROR "Neither WIN32 nor UNIX were defined")
#endif ()
#
#include_directories(${FMOD_INCLUDE_DIR})
#
#set(FMOD_LIB_DIR "lib/x64/fmod${BUILD_CONFIG_POSTFIX}_vc.lib")


