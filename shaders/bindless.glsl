layout (set = 0, binding = 0) uniform sampler2D bindless_color_textures[];
layout (set = 0, binding = 0) uniform sampler2D bindless_depth_textures[];
layout (set = 0, binding = 0) uniform samplerCube bindless_cubemap_textures[];
layout (set = 0, binding = 0) uniform sampler2DShadow bindless_shadowmap_textures[];

layout (set = 0, binding = 1, r16f) writeonly uniform image2D bindless_storage_image_r16f;

struct Material
{
    vec4 albedoFactor;

    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    vec3 emissiveFactor;

    bool useEmissiveMap;
    bool useAlbedoMap;
    bool useMRMap;
    bool useNormalMap;
    bool useOcclusionMap;

    uint albedoMapIndex;
    uint mrMapIndex;
    uint normalMapIndex;
    uint occlusionMapIndex;
    uint emissiveMapIndex;
};
layout (std140, set = 0, binding = 2) uniform Materials
{
    Material bindless_materials[64];
};

