set(FMOD_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(FMOD_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_library(FMOD INTERFACE)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(FMOD_DEBUG_POSTFIX L)
endif ()

if (WIN32)
    set(FMOD_LIBRARIES
            ${FMOD_LIB_DIR}/Windows/fmod${FMOD_DEBUG_POSTFIX}_vc.lib
            ${FMOD_LIB_DIR}/Windows/fmodstudio${FMOD_DEBUG_POSTFIX}_vc.lib)

    add_custom_target(COPY_FMOD_DLLS ALL
            COMMENT "Copying FMOD DLLs to Output"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FMOD_LIB_DIR}/Windows/fmod${FMOD_DEBUG_POSTFIX}.dll
            ${FMOD_LIB_DIR}/Windows/fmodstudio${FMOD_DEBUG_POSTFIX}.dll
            ${CMAKE_BINARY_DIR}
    )

    add_dependencies(FMOD COPY_FMOD_DLLS)
elseif (UNIX)
    set(FMOD_LIBRARIES
            ${FMOD_LIB_DIR}/Linux/libfmod${FMOD_DEBUG_POSTFIX}.so.14.4
            ${FMOD_LIB_DIR}/Linux/libfmodstudio${FMOD_DEBUG_POSTFIX}.so.14.4
    )
endif ()

target_link_libraries(FMOD INTERFACE ${FMOD_LIBRARIES})
target_include_directories(FMOD INTERFACE ${FMOD_INCLUDE_DIR})
