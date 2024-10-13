find_program(GLSLC glslc HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

if (NOT GLSLC)
    message(FATAL_ERROR "glslc not found at ${GLSLC}")
endif ()

set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SHADERS CONFIGURE_DEPENDS 
  SHADER_DIR/*.frag
  SHADER_DIR/*.vert
  SHADER_DIR/*.tesc
  SHADER_DIR/*.tese
  SHADER_DIR/*.geom
  SHADER_DIR/*.comp
)

set(SHADERS
        ${SHADER_DIR}/brdf_integration.frag
        ${SHADER_DIR}/brdf_integration.vert
        ${SHADER_DIR}/fullscreen.vert
        ${SHADER_DIR}/gaussian_blur.frag
        ${SHADER_DIR}/geom.frag
        ${SHADER_DIR}/geom.vert
        ${SHADER_DIR}/irradiance.frag
        ${SHADER_DIR}/irradiance.vert
        ${SHADER_DIR}/lighting.frag
        ${SHADER_DIR}/prefilter.frag
        ${SHADER_DIR}/prefilter.vert
        ${SHADER_DIR}/shadow.vert
        ${SHADER_DIR}/skydome.frag
        ${SHADER_DIR}/skydome.vert
        ${SHADER_DIR}/tonemapping.frag
)

set(SPIRV_BIN "${CMAKE_CURRENT_SOURCE_DIR}/bin")
file(MAKE_DIRECTORY ${SPIRV_BIN})

function(compile_shader shader)
    get_filename_component(FILE_NAME ${shader} NAME)
    set(SPIRV_OUTPUT "${SPIRV_BIN}/${FILE_NAME}.spv")

    add_custom_command(
            OUTPUT ${SPIRV_OUTPUT}
            COMMAND "${GLSLC}" ${shader} -o ${SPIRV_OUTPUT}
            COMMAND_EXPAND_LISTS
            COMMENT "Compiling ${shader} to SPIR-V"
            VERBATIM
    )

    list(APPEND SPIRV_SHADERS ${SPIRV_OUTPUT})
    set(SPIRV_SHADERS ${SPIRV_SHADERS} PARENT_SCOPE)
endfunction()

foreach (SHADER ${SHADERS})
    compile_shader(${SHADER})
endforeach ()

add_custom_target(Shaders ALL DEPENDS ${SPIRV_SHADERS})
