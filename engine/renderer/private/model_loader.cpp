#include "model_loader.hpp"

#include "animation.hpp"
#include "batch_buffer.hpp"
#include "ecs.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "log.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/material_resource_manager.hpp"
#include "resource_management/mesh_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "single_time_commands.hpp"
#include "timers.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <stb_image.h>

namespace detail
{

glm::vec3 ToVec3(const fastgltf::math::nvec3& gltf_vec)
{
    return { gltf_vec.x(), gltf_vec.y(), gltf_vec.z() };
}

glm::vec4 ToVec4(const fastgltf::math::nvec4& gltf_vec)
{
    return { gltf_vec.x(), gltf_vec.y(), gltf_vec.z(), gltf_vec.w() };
}

glm::mat4 ToMat4(const fastgltf::math::fmat4x4& gltf_mat)
{
    return glm::make_mat4(gltf_mat.data());
}

fastgltf::math::fmat4x4 ToFastGLTFMat4(const glm::mat4& glm_mat)
{
    fastgltf::math::fmat4x4 out {};
    std::copy(&glm_mat[0][0], &glm_mat[3][3], out.data());
    return out;
}

}
CPUModel ProcessModel(const fastgltf::Asset& gltf, const std::string_view name);

CPUModel ModelLoader::ExtractModelFromGltfFile(std::string_view path)
{
    fastgltf::GltfFileStream fileStream { path };

    if (!fileStream.isOpen())
        throw std::runtime_error("Path not found!");

    std::string_view directory = path.substr(0, path.find_last_of('/'));
    size_t offset = path.find_last_of('/') + 1;
    std::string_view name = path.substr(offset, path.find_last_of('.') - offset);

    auto loadedGltf = _parser.loadGltf(fileStream, directory,
        fastgltf::Options::DecomposeNodeMatrices | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages);

    if (!loadedGltf)
        throw std::runtime_error(getErrorMessage(loadedGltf.error()).data());

    fastgltf::Asset& gltf = loadedGltf.get();

    if (gltf.scenes.size() > 1)
        bblog::warn("GLTF contains more than one scene, but we only load one scene!");

    return ProcessModel(gltf, name);
}

glm::vec4 CalculateTangent(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2,
    glm::vec3 normal)
{
    glm::vec3 e1 = p1 - p0;
    glm::vec3 e2 = p2 - p0;

    float deltaU1 = uv1.x - uv0.x;
    float deltaV1 = uv1.y - uv0.y;
    float deltaU2 = uv2.x - uv0.x;
    float deltaV2 = uv2.y - uv0.y;

    float f = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);

    glm::vec3 tangent;
    tangent.x = f * (deltaV2 * e1.x - deltaV1 * e2.x);
    tangent.y = f * (deltaV2 * e1.y - deltaV1 * e2.y);
    tangent.z = f * (deltaV2 * e1.z - deltaV1 * e2.z);
    tangent = glm::normalize(tangent);

    glm::vec3 bitangent;
    bitangent.x = f * (-deltaU2 * e1.x + deltaU1 * e2.x);
    bitangent.y = f * (-deltaU2 * e1.y + deltaU1 * e2.y);
    bitangent.z = f * (-deltaU2 * e1.z + deltaU1 * e2.z);
    bitangent = glm::normalize(bitangent);

    float w = (glm::dot(glm::cross(normal, tangent), bitangent) < 0.0f) ? -1.0f : 1.0f;

    return glm::vec4 { tangent.x, tangent.y, tangent.z, w };
}

vk::PrimitiveTopology MapGltfTopology(fastgltf::PrimitiveType gltfTopology)
{
    switch (gltfTopology)
    {
    case fastgltf::PrimitiveType::Points:
        return vk::PrimitiveTopology::ePointList;
    case fastgltf::PrimitiveType::Lines:
        return vk::PrimitiveTopology::eLineList;
    case fastgltf::PrimitiveType::LineLoop:
        throw std::runtime_error("LineLoop isn't supported by Vulkan!");
    case fastgltf::PrimitiveType::LineStrip:
        return vk::PrimitiveTopology::eLineStrip;
    case fastgltf::PrimitiveType::Triangles:
        return vk::PrimitiveTopology::eTriangleList;
    case fastgltf::PrimitiveType::TriangleStrip:
        return vk::PrimitiveTopology::eTriangleStrip;
    case fastgltf::PrimitiveType::TriangleFan:
        return vk::PrimitiveTopology::eTriangleFan;
    default:
        throw std::runtime_error("Unsupported primitive type!");
    }
}

template <typename T>
void CalculateTangents(CPUMesh<T>& stagingPrimitive)
{
    uint32_t triangleCount = stagingPrimitive.indices.size() > 0 ? stagingPrimitive.indices.size() / 3 : stagingPrimitive.vertices.size() / 3;
    for (size_t i = 0; i < triangleCount; ++i)
    {
        std::array<T*, 3> triangle = {};
        if (stagingPrimitive.indices.size() > 0)
        {
            std::array<uint32_t, 3> indices = {};

            indices[0] = stagingPrimitive.indices[(i * 3 + 0)];
            indices[1] = stagingPrimitive.indices[(i * 3 + 1)];
            indices[2] = stagingPrimitive.indices[(i * 3 + 2)];

            triangle = {
                &stagingPrimitive.vertices[indices[0]],
                &stagingPrimitive.vertices[indices[1]],
                &stagingPrimitive.vertices[indices[2]]
            };
        }
        else
        {
            triangle = {
                &stagingPrimitive.vertices[i * 3 + 0],
                &stagingPrimitive.vertices[i * 3 + 1],
                &stagingPrimitive.vertices[i * 3 + 2]
            };
        }

        glm::vec4 tangent = CalculateTangent(triangle[0]->position, triangle[1]->position, triangle[2]->position,
            triangle[0]->texCoord, triangle[1]->texCoord, triangle[2]->texCoord,
            triangle[0]->normal);

        triangle[0]->tangent += tangent;
        triangle[1]->tangent += tangent;
        triangle[2]->tangent += tangent;
    }

    for (size_t i = 0; i < stagingPrimitive.vertices.size(); ++i)
    {
        glm::vec3 tangent = stagingPrimitive.vertices[i].tangent;
        tangent = glm::normalize(tangent);
        stagingPrimitive.vertices[i].tangent = glm::vec4 { tangent.x, tangent.y, tangent.z, stagingPrimitive.vertices[i].tangent.w };
    }

    if (std::any_of(stagingPrimitive.vertices.begin(), stagingPrimitive.vertices.end(), [](const auto& v)
            { return std::isnan(v.tangent.x); }))
    {
        for (auto& vertex : stagingPrimitive.vertices)
        {
            vertex.tangent = glm::vec4 { 1.0f, 0.0f, 0.0f, 1.0f };
        }
    }
}

template <typename T>
void CalculateNormals(CPUMesh<T>& mesh)
{
    for (size_t i = 0; i < mesh.indices.size(); i += 3)
    {
        uint32_t idx0 = mesh.indices[i];
        uint32_t idx1 = mesh.indices[i + 1];
        uint32_t idx2 = mesh.indices[i + 2];

        glm::vec3& v0 = mesh.vertices[idx0].position;
        glm::vec3& v1 = mesh.vertices[idx1].position;
        glm::vec3& v2 = mesh.vertices[idx2].position;

        // Compute edges
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        // Compute face normal (cross product)
        glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

        // Accumulate the face normal into vertex normals
        mesh.vertices[idx0].normal += faceNormal;
        mesh.vertices[idx1].normal += faceNormal;
        mesh.vertices[idx2].normal += faceNormal;
    }

    // Normalize the normals at each vertex
    for (auto& vertex : mesh.vertices)
    {
        vertex.normal = glm::normalize(vertex.normal);
    }
}

template <typename T, typename U>
void AssignAttribute(T& vertexAttribute, uint32_t index, const fastgltf::Attribute* attribute, const fastgltf::Primitive& gltfPrimitive, const fastgltf::Asset& gltf)
{
    if (attribute != gltfPrimitive.attributes.cend())
    {
        const auto& accessor = gltf.accessors[attribute->accessorIndex];
        auto value = fastgltf::getAccessorElement<U>(gltf, accessor, index);
        vertexAttribute = *reinterpret_cast<T*>(&value);
    }
}

template <typename T>
void ProcessVertices(std::vector<T>& vertices, const fastgltf::Primitive& gltfPrimitive, const fastgltf::Asset& gltf, Vec3Range& boundingBox, float& boundingRadius);

template <>
void ProcessVertices<Vertex>(std::vector<Vertex>& vertices, const fastgltf::Primitive& gltfPrimitive, const fastgltf::Asset& gltf, Vec3Range& boundingBox, float& boundingRadius)
{
    uint32_t vertexCount = gltf.accessors[gltfPrimitive.findAttribute("POSITION")->accessorIndex].count;
    vertices = std::vector<Vertex>(vertexCount);

    const fastgltf::Attribute* positionAttribute = gltfPrimitive.findAttribute("POSITION");
    const fastgltf::Attribute* normalAttribute = gltfPrimitive.findAttribute("NORMAL");
    const fastgltf::Attribute* tangentAttribute = gltfPrimitive.findAttribute("TANGENT");
    const fastgltf::Attribute* texCoordAttribute = gltfPrimitive.findAttribute("TEXCOORD_0");

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        AssignAttribute<glm::vec3, fastgltf::math::fvec3>(vertices[i].position, i, positionAttribute, gltfPrimitive, gltf);
        AssignAttribute<glm::vec3, fastgltf::math::fvec3>(vertices[i].normal, i, normalAttribute, gltfPrimitive, gltf);
        AssignAttribute<glm::vec4, fastgltf::math::fvec4>(vertices[i].tangent, i, tangentAttribute, gltfPrimitive, gltf);
        AssignAttribute<glm::vec2, fastgltf::math::fvec2>(vertices[i].texCoord, i, texCoordAttribute, gltfPrimitive, gltf);

        boundingBox.min = glm::min(boundingBox.min, vertices[i].position);
        boundingBox.max = glm::max(boundingBox.max, vertices[i].position);
        boundingRadius = glm::max(boundingRadius, glm::distance2(glm::vec3 { 0.0f }, vertices[i].position));
    }

    boundingRadius = glm::sqrt(boundingRadius);
}

template <>
void ProcessVertices<SkinnedVertex>(std::vector<SkinnedVertex>& vertices, const fastgltf::Primitive& gltfPrimitive, const fastgltf::Asset& gltf, Vec3Range& boundingBox, float& boundingRadius)
{
    uint32_t vertexCount = gltf.accessors[gltfPrimitive.findAttribute("POSITION")->accessorIndex].count;
    vertices = std::vector<SkinnedVertex>(vertexCount);

    const fastgltf::Attribute* positionAttribute = gltfPrimitive.findAttribute("POSITION");
    const fastgltf::Attribute* normalAttribute = gltfPrimitive.findAttribute("NORMAL");
    const fastgltf::Attribute* tangentAttribute = gltfPrimitive.findAttribute("TANGENT");
    const fastgltf::Attribute* texCoordAttribute = gltfPrimitive.findAttribute("TEXCOORD_0");
    const fastgltf::Attribute* jointsAttribute = gltfPrimitive.findAttribute("JOINTS_0");
    const fastgltf::Attribute* weightsAttribute = gltfPrimitive.findAttribute("WEIGHTS_0");

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        AssignAttribute<glm::vec3, fastgltf::math::fvec3>(vertices[i].position, i, positionAttribute, gltfPrimitive, gltf);
        AssignAttribute<glm::vec3, fastgltf::math::fvec3>(vertices[i].normal, i, normalAttribute, gltfPrimitive, gltf);
        AssignAttribute<glm::vec4, fastgltf::math::fvec4>(vertices[i].tangent, i, tangentAttribute, gltfPrimitive, gltf);
        AssignAttribute<glm::vec2, fastgltf::math::fvec2>(vertices[i].texCoord, i, texCoordAttribute, gltfPrimitive, gltf);
        AssignAttribute<glm::vec4, fastgltf::math::fvec4>(vertices[i].joints, i, jointsAttribute, gltfPrimitive, gltf);
        AssignAttribute<glm::vec4, fastgltf::math::fvec4>(vertices[i].weights, i, weightsAttribute, gltfPrimitive, gltf);

        boundingBox.min = glm::min(boundingBox.min, vertices[i].position);
        boundingBox.max = glm::max(boundingBox.max, vertices[i].position);
        boundingRadius = glm::max(boundingRadius, glm::distance2(glm::vec3 { 0.0f }, vertices[i].position));
    }

    boundingRadius = glm::sqrt(boundingRadius);
}

template <typename T>
CPUMesh<T> ProcessPrimitive(const fastgltf::Primitive& gltfPrimitive, const fastgltf::Asset& gltf)
{
    CPUMesh<T> mesh {};

    assert(MapGltfTopology(gltfPrimitive.type) == vk::PrimitiveTopology::eTriangleList && "Only triangle list topology is supported!");
    if (gltfPrimitive.materialIndex.has_value())
        mesh.materialIndex = gltfPrimitive.materialIndex.value();

    ProcessVertices(mesh.vertices, gltfPrimitive, gltf, mesh.boundingBox, mesh.boundingRadius);

    if (gltfPrimitive.indicesAccessor.has_value())
    {
        auto& accessor = gltf.accessors[gltfPrimitive.indicesAccessor.value()];
        if (!accessor.bufferViewIndex.has_value())
            throw std::runtime_error("Failed retrieving buffer view index from accessor!");
        auto& bufferView = gltf.bufferViews[accessor.bufferViewIndex.value()];
        auto& buffer = gltf.buffers[bufferView.bufferIndex];
        auto& bufferBytes = std::get<fastgltf::sources::Array>(buffer.data);

        mesh.indices = std::vector<uint32_t>(accessor.count);

        const std::byte* attributeBufferStart = bufferBytes.bytes.data() + bufferView.byteOffset + accessor.byteOffset;

        if (accessor.componentType == fastgltf::ComponentType::UnsignedInt && (!bufferView.byteStride.has_value() || bufferView.byteStride.value() == 0))
        {
            std::memcpy(mesh.indices.data(), attributeBufferStart, mesh.indices.size() * sizeof(uint32_t));
        }
        else
        {
            uint32_t gltfIndexTypeSize = fastgltf::getComponentByteSize(accessor.componentType);
            for (size_t i = 0; i < accessor.count; ++i)
            {
                const std::byte* element = attributeBufferStart + i * gltfIndexTypeSize + (bufferView.byteStride.has_value() ? bufferView.byteStride.value() : 0);
                uint32_t* indexPtr = mesh.indices.data() + i;
                std::memcpy(indexPtr, element, gltfIndexTypeSize);
            }
        }
    }
    else
    {
        // Generate indices manually
        mesh.indices.reserve(mesh.vertices.size());
        for (size_t i = 0; i < mesh.vertices.size(); ++i)
        {
            mesh.indices.emplace_back(i);
        }
    }

    const fastgltf::Attribute* normalAttribute = gltfPrimitive.findAttribute("NORMAL");
    if (normalAttribute == gltfPrimitive.attributes.cend())
    {
        CalculateNormals(mesh);
    }

    const fastgltf::Attribute* tangentAttribute = gltfPrimitive.findAttribute("TANGENT");
    const fastgltf::Attribute* texCoordAttribute = gltfPrimitive.findAttribute("TEXCOORD_0");
    if (tangentAttribute == gltfPrimitive.attributes.cend() && texCoordAttribute != gltfPrimitive.attributes.cend())
    {
        CalculateTangents<T>(mesh);
    }

    return mesh;
}

CPUImage ProcessImage(const fastgltf::Image& gltfImage, const fastgltf::Asset& gltf, std::vector<std::byte>& data,
    std::string_view name)
{
    CPUImage cpuImage {};

    std::visit(fastgltf::visitor {
                   [](MAYBE_UNUSED auto& arg) {},
                   [&](const fastgltf::sources::URI& filePath)
                   {
                       assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
                       assert(filePath.uri.isLocalPath()); // We're only capable of loading local files.
                       int32_t width, height, nrChannels;

                       const std::string path(filePath.uri.path().begin(), filePath.uri.path().end()); // Thanks C++.
                       stbi_uc* stbiData = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                       if (!stbiData)
                           bblog::error("Failed loading data from STBI at path: {}", path);

                       data = std::vector<std::byte>(width * height * 4);
                       std::memcpy(data.data(), reinterpret_cast<std::byte*>(stbiData), data.size());

                       cpuImage.SetName(name).SetSize(width, height).SetData(std::move(data)).SetFlags(vk::ImageUsageFlagBits::eSampled).SetFormat(vk::Format::eR8G8B8A8Unorm);

                       stbi_image_free(stbiData);
                   },
                   [&](const fastgltf::sources::Array& vector)
                   {
                       int32_t width, height, nrChannels;
                       stbi_uc* stbiData = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vector.bytes.data()),
                           static_cast<int32_t>(vector.bytes.size()), &width, &height,
                           &nrChannels, 4);

                       data = std::vector<std::byte>(width * height * 4);
                       std::memcpy(data.data(), reinterpret_cast<std::byte*>(stbiData), data.size());

                       cpuImage.SetName(name).SetSize(width, height).SetData(std::move(data)).SetFlags(vk::ImageUsageFlagBits::eSampled).SetFormat(vk::Format::eR8G8B8A8Unorm);

                       stbi_image_free(stbiData);
                   },
                   [&](const fastgltf::sources::BufferView& view)
                   {
                       auto& bufferView = gltf.bufferViews[view.bufferViewIndex];
                       auto& buffer = gltf.buffers[bufferView.bufferIndex];

                       std::visit(
                           fastgltf::visitor { // We only care about VectorWithMime here, because we specify LoadExternalBuffers, meaning
                               // all buffers are already loaded into a vector.
                               [](MAYBE_UNUSED auto& arg) {},
                               [&](const fastgltf::sources::Array& vector)
                               {
                                   int32_t width, height, nrChannels;
                                   stbi_uc* stbiData = stbi_load_from_memory(
                                       reinterpret_cast<const stbi_uc*>(vector.bytes.data() + bufferView.byteOffset),
                                       static_cast<int32_t>(bufferView.byteLength), &width, &height, &nrChannels,
                                       4);

                                   data = std::vector<std::byte>(width * height * 4);
                                   std::memcpy(data.data(), reinterpret_cast<std::byte*>(stbiData), data.size());

                                   cpuImage.SetName(name).SetSize(width, height).SetData(std::move(data)).SetFlags(vk::ImageUsageFlagBits::eSampled).SetFormat(vk::Format::eR8G8B8A8Unorm);

                                   stbi_image_free(stbiData);
                               } },
                           buffer.data);
                   },
               },
        gltfImage.data);

    return cpuImage;
}

StagingAnimationChannels LoadAnimations(const fastgltf::Asset& gltf)
{
    StagingAnimationChannels stagingAnimationChannels {};

    stagingAnimationChannels.animation.name = gltf.animations[0].name;

    for (const auto& channel : gltf.animations[0].channels)
    {
        // If there is no node, the channel is invalid.
        if (!channel.nodeIndex.has_value())
        {
            continue;
        }

        AnimationChannel* spline { nullptr };
        const auto it = std::find(stagingAnimationChannels.nodeIndices.begin(), stagingAnimationChannels.nodeIndices.end(), channel.nodeIndex.value());

        if (it == stagingAnimationChannels.nodeIndices.end())
        {
            stagingAnimationChannels.nodeIndices.emplace_back(channel.nodeIndex.value());
            spline = &stagingAnimationChannels.animationChannels.emplace_back();
        }
        else
        {
            spline = &stagingAnimationChannels.animationChannels[std::distance(stagingAnimationChannels.nodeIndices.begin(), it)];
        }

        const auto& sampler = gltf.animations[0].samplers[channel.samplerIndex];

        assert(sampler.interpolation == fastgltf::AnimationInterpolation::Linear && "Only linear interpolation supported!");

        std::span<const float> timestamps;

        {
            const auto& accessor = gltf.accessors[sampler.inputAccessor];
            const auto& bufferView = gltf.bufferViews[accessor.bufferViewIndex.value()];
            const auto& buffer = gltf.buffers[bufferView.bufferIndex];
            assert(!accessor.sparse.has_value() && "No support for sparse accesses");
            assert(!bufferView.byteStride.has_value() && "No support for byte stride view");

            const std::byte* data = std::get<fastgltf::sources::Array>(buffer.data).bytes.data() + bufferView.byteOffset + accessor.byteOffset;
            timestamps = std::span<const float> { reinterpret_cast<const float*>(data), accessor.count };

            if (stagingAnimationChannels.animation.duration < timestamps.back())
            {
                stagingAnimationChannels.animation.duration = timestamps.back();
            }
        }
        {
            const auto& accessor = gltf.accessors[sampler.outputAccessor];
            const auto& bufferView = gltf.bufferViews[accessor.bufferViewIndex.value()];
            const auto& buffer = gltf.buffers[bufferView.bufferIndex];
            assert(!accessor.sparse.has_value() && "No support for sparse accesses");
            assert(!bufferView.byteStride.has_value() && "No support for byte stride view");

            const std::byte* data = std::get<fastgltf::sources::Array>(buffer.data).bytes.data() + bufferView.byteOffset + accessor.byteOffset;
            if (channel.path == fastgltf::AnimationPath::Translation)
            {
                spline->translation = AnimationSpline<Translation> {};

                std::span<const Translation> output { reinterpret_cast<const Translation*>(data), accessor.count };

                spline->translation.value().values = std::vector<Translation> { output.begin(), output.end() };
                spline->translation.value().timestamps = std::vector<float> { timestamps.begin(), timestamps.end() };
            }
            else if (channel.path == fastgltf::AnimationPath::Rotation)
            {
                spline->rotation = AnimationSpline<Rotation> {};

                std::span<const Rotation> output { reinterpret_cast<const Rotation*>(data), bufferView.byteLength / sizeof(Rotation) };

                spline->rotation.value().values = std::vector<Rotation> { output.begin(), output.end() };
                spline->rotation.value().timestamps = std::vector<float> { timestamps.begin(), timestamps.end() };
            }
            else if (channel.path == fastgltf::AnimationPath::Scale)
            {
                spline->scaling = AnimationSpline<Scale> {};

                std::span<const Scale> output { reinterpret_cast<const Scale*>(data), accessor.count };

                spline->scaling.value().values = std::vector<Scale> { output.begin(), output.end() };
                spline->scaling.value().timestamps = std::vector<float> { timestamps.begin(), timestamps.end() };
            }
        }
    }

    return stagingAnimationChannels;
}

CPUModel::CPUMaterial ProcessMaterial(const fastgltf::Material& gltfMaterial, const std::vector<fastgltf::Texture>& gltfTextures)
{
    auto MapTextureIndexToImageIndex = [](uint32_t textureIndex, const std::vector<fastgltf::Texture>& gltfTextures) -> uint32_t
    {
        return gltfTextures[textureIndex].imageIndex.value();
    };

    CPUModel::CPUMaterial material {};

    if (gltfMaterial.pbrData.baseColorTexture.has_value())
    {
        uint32_t textureIndex = MapTextureIndexToImageIndex(gltfMaterial.pbrData.baseColorTexture.value().textureIndex, gltfTextures);
        material.albedoMap = textureIndex;
    }

    if (gltfMaterial.pbrData.metallicRoughnessTexture.has_value())
    {
        uint32_t textureIndex = MapTextureIndexToImageIndex(gltfMaterial.pbrData.metallicRoughnessTexture.value().textureIndex, gltfTextures);
        material.metallicRoughnessMap = textureIndex;
    }

    if (gltfMaterial.normalTexture.has_value())
    {
        uint32_t textureIndex = MapTextureIndexToImageIndex(gltfMaterial.normalTexture.value().textureIndex, gltfTextures);
        material.normalMap = textureIndex;
    }

    if (gltfMaterial.occlusionTexture.has_value())
    {
        uint32_t textureIndex = MapTextureIndexToImageIndex(gltfMaterial.occlusionTexture.value().textureIndex, gltfTextures);
        material.occlusionMap = textureIndex;
    }

    if (gltfMaterial.emissiveTexture.has_value())
    {
        uint32_t textureIndex = MapTextureIndexToImageIndex(gltfMaterial.emissiveTexture.value().textureIndex, gltfTextures);
        material.emissiveMap = textureIndex;
    }

    material.albedoFactor = detail::ToVec4(gltfMaterial.pbrData.baseColorFactor);
    material.metallicFactor = gltfMaterial.pbrData.metallicFactor;
    material.roughnessFactor = gltfMaterial.pbrData.roughnessFactor;
    material.normalScale = gltfMaterial.normalTexture.has_value() ? gltfMaterial.normalTexture.value().scale : 0.0f;
    material.emissiveFactor = detail::ToVec3(gltfMaterial.emissiveFactor);
    material.occlusionStrength = gltfMaterial.occlusionTexture.has_value()
        ? gltfMaterial.occlusionTexture.value().strength
        : 1.0f;

    return material;
}

Hierarchy::Node RecurseHierarchy(const fastgltf::Node& gltfNode, uint32_t gltfNodeIndex, CPUModel& model, const fastgltf::Asset& gltf, const StagingAnimationChannels& animationChannels, const std::unordered_multimap<uint32_t, std::pair<MeshType, uint32_t>>& meshLUT)
{
    Hierarchy::Node node {};

    if (gltfNode.meshIndex.has_value())
    {
        // Add meshes as children, since glTF assumes 1 node -> 1 mesh -> >1 primitives
        // But now we want 1 node -> mesh (mesh == primitive)
        auto range = meshLUT.equal_range(gltfNode.meshIndex.value());
        for (auto it = range.first; it != range.second; ++it)
        {
            node.children.emplace_back(Hierarchy::Node { "mesh node", glm::identity<glm::mat4>(), it->second });
        }

        if (gltfNode.skinIndex.has_value())
        {
            const auto& skin = gltf.skins[gltfNode.skinIndex.value()];
            int x = 0;
        }
    }

    fastgltf::math::fmat4x4 gltfTransform = fastgltf::getTransformMatrix(gltfNode);
    node.transform = detail::ToMat4(gltfTransform);
    node.name = gltfNode.name;

    for (size_t i = 0; i < animationChannels.nodeIndices.size(); i++)
    {
        if (animationChannels.nodeIndices[i] == gltfNodeIndex)
        {
            node.animationChannel = animationChannels.animationChannels[i];
            break;
        }
    }

    for (const auto& skin : gltf.skins)
    {
        auto it = std::find(skin.joints.begin(), skin.joints.end(), gltfNodeIndex);
        if (it != skin.joints.end())
        {
            fastgltf::math::fmat4x4 inverseBindMatrix = fastgltf::getAccessorElement<fastgltf::math::fmat4x4>(gltf, gltf.accessors[skin.inverseBindMatrices.value()], std::distance(skin.joints.begin(), it));
            node.joint = Hierarchy::Joint {
                gltfNodeIndex == skin.skeleton.has_value() ? static_cast<bool>(skin.skeleton.value()) : false,
                entt::null,
                *reinterpret_cast<glm::mat4x4*>(&inverseBindMatrix),
                static_cast<uint32_t>(std::distance(skin.joints.begin(), it))
            };
            break;
        }
    }

    for (size_t i : gltfNode.children)
    {
        node.children.emplace_back(RecurseHierarchy(gltf.nodes[i], i, model, gltf, animationChannels, meshLUT));
    }

    return node;
}

CPUModel ProcessModel(const fastgltf::Asset& gltf, const std::string_view name)
{
    CPUModel model {};

    // Extract texture data
    std::vector<std::vector<std::byte>> textureData(gltf.images.size());

    for (size_t i = 0; i < gltf.images.size(); ++i)
    {
        const CPUImage image = ProcessImage(gltf.images[i], gltf, textureData[i], name);
        model.textures.emplace_back(image);
    }

    // Extract material data
    for (auto& gltfMaterial : gltf.materials)
    {
        const CPUModel::CPUMaterial material = ProcessMaterial(gltfMaterial, gltf.textures);
        model.materials.emplace_back(material);
    }

    std::unordered_multimap<uint32_t, std::pair<MeshType, uint32_t>> meshLUT {};

    // Extract mesh data
    uint32_t counter { 0 };
    for (auto& gltfMesh : gltf.meshes)
    {
        for (const auto& gltfPrimitive : gltfMesh.primitives)
        {
            if (gltfPrimitive.findAttribute("JOINTS_0") != gltfPrimitive.attributes.cend())
            {
                CPUMesh<SkinnedVertex> primitive = ProcessPrimitive<SkinnedVertex>(gltfPrimitive, gltf);
                model.skinnedMeshes.emplace_back(primitive);

                meshLUT.insert({ counter, std::pair(MeshType::eSKINNED, model.skinnedMeshes.size() - 1) });
            }
            else
            {
                CPUMesh<Vertex> primitive = ProcessPrimitive<Vertex>(gltfPrimitive, gltf);
                model.meshes.emplace_back(primitive);

                meshLUT.insert({ counter, std::pair(MeshType::eSTATIC, model.meshes.size() - 1) });
            }
        }
        ++counter;
    }

    StagingAnimationChannels animations {};
    if (!gltf.animations.empty())
    {
        animations = LoadAnimations(gltf);
        model.animation = animations.animation;
    }

    Hierarchy::Node baseNode;
    baseNode.name = name;

    for (size_t i = 0; i < gltf.scenes[0].nodeIndices.size(); ++i)
    {
        uint32_t index = gltf.scenes[0].nodeIndices[i];
        const auto& gltfNode { gltf.nodes[index] };
        baseNode.children.emplace_back(RecurseHierarchy(gltfNode, index, model, gltf, animations, meshLUT));
    }
    model.hierarchy.baseNodes.emplace_back(std::move(baseNode));

    return model;
}

void ModelLoader::ReadGeometrySize(std::string_view path, uint32_t& vertexBufferSize, uint32_t& indexBufferSize)
{
    vertexBufferSize = 0;
    indexBufferSize = 0;

    fastgltf::GltfFileStream fileStream { path };

    if (!fileStream.isOpen())
        throw std::runtime_error("Path not found!");

    std::string_view directory = path.substr(0, path.find_last_of('/'));
    auto loadedGltf = _parser.loadGltf(fileStream, directory,
        fastgltf::Options::DecomposeNodeMatrices | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages);

    if (!loadedGltf)
        throw std::runtime_error(getErrorMessage(loadedGltf.error()).data());

    fastgltf::Asset& gltf = loadedGltf.get();

    for (const auto& mesh : gltf.meshes)
    {
        for (const auto& primitive : mesh.primitives)
        {
            const auto& vertexAccessor = gltf.accessors[primitive.attributes[0].accessorIndex];
            uint32_t vertexCount = vertexAccessor.count;

            vertexBufferSize += vertexCount * sizeof(Vertex);

            if (primitive.indicesAccessor.has_value())
            {
                const auto& indexAccessor = gltf.accessors[primitive.indicesAccessor.value()];
                uint32_t indexCount = indexAccessor.count;

                indexBufferSize += indexCount * sizeof(uint32_t);
            }
        }
    }
}
