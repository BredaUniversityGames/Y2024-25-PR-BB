layout (set = 0, binding = 0) uniform sampler2D bindless_color_textures[];
layout (set = 0, binding = 1) uniform sampler2D bindless_depth_textures[];
layout (set = 0, binding = 2) uniform samplerCube bindless_cubemap_textures[];
layout (set = 0, binding = 3) uniform sampler2DShadow bindless_shadowmap_textures[];

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
layout (std140, set = 0, binding = 4) buffer Materials
{
    Material bindless_materials[];
};

