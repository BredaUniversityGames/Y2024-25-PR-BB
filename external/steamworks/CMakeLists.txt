set(STEAMWORKS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(STEAMWORKS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_library(Steamworks INTERFACE)

target_include_directories(Steamworks INTERFACE ${STEAMWORKS_INCLUDE_DIR})

if (WIN32)
    set(STEAMWORKS_LIBRARIES "${STEAMWORKS_LIB_DIR}/Windows/steam_api64.dll")
elseif (UNIX)
    set(STEAMWORKS_LIBRARIES "${STEAMWORKS_LIB_DIR}/Linux/libsteam_api.so")
endif ()

target_link_libraries(Steamworks INTERFACE ${STEAMWORKS_LIBRARIES})