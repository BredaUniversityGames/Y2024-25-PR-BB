set(STEAMWORKS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(STEAMWORKS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_library(Steamworks INTERFACE)

if (WIN32)
    set(STEAMWORKS_LIBRARIES "${STEAMWORKS_LIB_DIR}/Windows/steam_api64.lib")

    add_custom_target(COPY_STEAMWORK_DLL ALL
            COMMENT "Copying Steamworks DLL to Output"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${STEAMWORKS_LIB_DIR}/Windows/steam_api64.dll
            ${CMAKE_BINARY_DIR}
    )

    add_dependencies(Steamworks COPY_STEAMWORK_DLL)
elseif (UNIX)
    set(STEAMWORKS_LIBRARIES "${STEAMWORKS_LIB_DIR}/Linux/libsteam_api.so")
endif ()

target_include_directories(Steamworks INTERFACE ${STEAMWORKS_INCLUDE_DIR})
target_link_libraries(Steamworks INTERFACE ${STEAMWORKS_LIBRARIES})