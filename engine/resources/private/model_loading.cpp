#include "model_loading.hpp"

#include "animation.hpp"
#include "batch_buffer.hpp"
#include "ecs_module.hpp"
#include "file_io.hpp"
#include "lib/include_fastgltf.hpp"
#include "log.hpp"
#include "math_util.hpp"
#include "physics/shape_factory.hpp"
#include "profile_macros.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/mesh_resource_manager.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <stb_image.h>

constexpr static auto DEFAULT_LOAD_FLAGS = fastgltf::Options::DecomposeNodeMatrices | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages;
static fastgltf::Parser parser = fastgltf::Parser(fastgltf::Extensions::KHR_lights_punctual | fastgltf::Extensions::KHR_texture_transform);

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

fastgltf::Asset LoadFastGLTFAsset(std::string_view path)
{
    ZoneScoped;
    std::string zone = std::string(path) + " fastgltf parse";
    ZoneName(zone.c_str(), 128);

    fastgltf::GltfFileStream fileStream { path };

    if (!fileStream.isOpen())
        throw std::runtime_error("Path not found!");

    std::string_view directory = path.substr(0, path.find_last_of('/'));

    auto loadedGltf = parser.loadGltf(fileStream, directory, DEFAULT_LOAD_FLAGS);
    if (!loadedGltf)
        throw std::runtime_error(getErrorMessage(loadedGltf.error()).data());

    auto gltf = std::move(loadedGltf.get());

    if (gltf.scenes.size() > 1)
        bblog::warn("GLTF contains more than one scene, but we only load one scene!");

    return gltf;
}

CPUImage ProcessImage(const fastgltf::Asset& asset, const fastgltf::Image& gltfImage)
{
    ZoneScopedN("Image Loading");

    CPUImage out {};
    out
        .SetFlags(vk::ImageUsageFlagBits::eSampled)
        .SetFormat(vk::Format::eR8G8B8A8Unorm)
        .SetName(gltfImage.name);

    if (auto* view = std::get_if<fastgltf::sources::BufferView>(&gltfImage.data))
    {
        auto& bufferView = asset.bufferViews[view->bufferViewIndex];
        auto& buffer = asset.buffers[bufferView.bufferIndex];

        if (auto* imageData = std::get_if<fastgltf::sources::Array>(&buffer.data))
        {
            int32_t width, height, nrChannels;
            stbi_uc* stbiData = nullptr;
            {
                ZoneScopedN("STB Image step");
                stbiData = stbi_load_from_memory(
                    reinterpret_cast<const stbi_uc*>(imageData->bytes.data() + bufferView.byteOffset),
                    static_cast<int32_t>(bufferView.byteLength), &width, &height, &nrChannels,
                    4);
            }

            {
                ZoneScopedN("Copy data step");
                std::vector<std::byte> data = std::vector<std::byte>(width * height * 4);
                std::memcpy(data.data(), std::bit_cast<std::byte*>(stbiData), data.size());

                out
                    .SetSize(width, height)
                    .SetData(std::move(data))
                    .SetMips(std::floor(std::log2(std::max(width, height))));
            }

            stbi_image_free(stbiData);
        }
        else
        {
            throw std::runtime_error("Unhandled image gltf type");
        }
    }
    else
    {
        throw std::runtime_error("Unhandled image gltf type");
    }

    return out;
}

// Uses mesh colliders from Jolt
JPH::ShapeRefC ProcessMeshIntoCollider(const CPUMesh<Vertex>& mesh)
{
    std::vector<glm::vec3> vertices;
    for (auto vertex : mesh.vertices)
    {
        vertices.emplace_back(vertex.position.x, vertex.position.y, vertex.position.z);
    }

    return ShapeFactory::MakeMeshHullShape(vertices, mesh.indices);
}

}

CPUModel ProcessModel(const fastgltf::Asset& gltf, const std::string_view name);

CPUModel ModelLoading::LoadGLTF(std::string_view path)
{
    ZoneScoped;

    std::string zone = std::string(path) + " Model Extraction";
    ZoneName(zone.c_str(), 128);

    fastgltf::GltfFileStream fileStream { path };

    if (!fileStream.isOpen())
        throw std::runtime_error("Path not found!");

    std::string_view directory = path.substr(0, path.find_last_of('/'));
    size_t offset = path.find_last_of('/') + 1;
    std::string_view name = path.substr(offset, path.find_last_of('.') - offset);

    fastgltf::Asset gltf {};

    {
        ZoneScoped;

        std::string zone = std::string(path) + " FastGLTF parse";
        ZoneName(zone.c_str(), 128);

        auto loadedGltf = parser.loadGltf(fileStream, directory, DEFAULT_LOAD_FLAGS);
        if (!loadedGltf)
            throw std::runtime_error(getErrorMessage(loadedGltf.error()).data());

        gltf = std::move(loadedGltf.get());
    }

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

        glm::vec4 tangent { 1.0f, 0.0f, 0.0f, 1.0f };
        // Check if all tex coords are different, otherwise just keep default.
        if (triangle[0]->texCoord != triangle[1]->texCoord && triangle[0]->texCoord != triangle[2]->texCoord && triangle[1]->texCoord != triangle[2]->texCoord)
        {
            tangent = CalculateTangent(triangle[0]->position, triangle[1]->position, triangle[2]->position,
                triangle[0]->texCoord, triangle[1]->texCoord, triangle[2]->texCoord,
                triangle[0]->normal);
        }

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
void ProcessVertices(std::vector<T>& vertices, const fastgltf::Primitive& gltfPrimitive, const fastgltf::Asset& gltf, math::Vec3Range& boundingBox, float& boundingRadius);

template <>
void ProcessVertices<Vertex>(std::vector<Vertex>& vertices, const fastgltf::Primitive& gltfPrimitive, const fastgltf::Asset& gltf, math::Vec3Range& boundingBox, float& boundingRadius)
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

        // Make sure tangent is perpendicular to normal.
        glm::vec3 tangent = glm::vec3 { vertices[i].tangent };
        if (glm::cross(vertices[i].normal, glm::vec3 { tangent }) == glm::vec3 { 0.0f })
        {
            tangent = glm::cross(vertices[i].normal, glm::vec3 { 0.0f, 1.0f, 0.0f });

            vertices[i].tangent = glm::vec4 { tangent, vertices[i].tangent.w };
        }

        boundingBox.min = glm::min(boundingBox.min, vertices[i].position);
        boundingBox.max = glm::max(boundingBox.max, vertices[i].position);
        boundingRadius = glm::max(boundingRadius, glm::distance2(glm::vec3 { 0.0f }, vertices[i].position));
    }

    boundingRadius = glm::sqrt(boundingRadius);
}

template <>
void ProcessVertices<SkinnedVertex>(std::vector<SkinnedVertex>& vertices, const fastgltf::Primitive& gltfPrimitive, const fastgltf::Asset& gltf, math::Vec3Range& boundingBox, float& boundingRadius)
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
    mesh.boundingBox.min = glm::vec3 { std::numeric_limits<float>::max() };
    mesh.boundingBox.max = glm::vec3 { std::numeric_limits<float>::lowest() };

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
    ZoneScopedN("Image Loading");

    std::visit(fastgltf::visitor {
                   [](MAYBE_UNUSED auto& arg) {},
                   [&](const fastgltf::sources::URI& filePath)
                   {
                       assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
                       assert(filePath.uri.isLocalPath()); // We're only capable of loading local files.

                       int32_t width, height, nrChannels;

                       const std::string path(filePath.uri.path().begin(), filePath.uri.path().end()); // Thanks C++.
                       auto stream = fileIO::OpenReadStream(path);
                       stbi_uc* stbiData = fileIO::LoadImageFromIfstream(stream.value(), &width, &height, &nrChannels, 4);
                       if (!stbiData)
                           bblog::error("Failed loading data from STBI at path: {}", path);

                       data = std::vector<std::byte>(width * height * 4);
                       std::memcpy(data.data(), reinterpret_cast<std::byte*>(stbiData), data.size());

                       cpuImage
                           .SetName(name)
                           .SetSize(width, height)
                           .SetData(std::move(data))
                           .SetFlags(vk::ImageUsageFlagBits::eSampled)
                           .SetFormat(vk::Format::eR8G8B8A8Unorm)
                           .SetMips(std::floor(std::log2(std::max(width, height))));

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

                       cpuImage
                           .SetName(name)
                           .SetSize(width, height)
                           .SetData(std::move(data))
                           .SetFlags(vk::ImageUsageFlagBits::eSampled)
                           .SetFormat(vk::Format::eR8G8B8A8Unorm)
                           .SetMips(std::floor(std::log2(std::max(width, height))));

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

                                   cpuImage
                                       .SetName(name)
                                       .SetSize(width, height)
                                       .SetData(std::move(data))
                                       .SetFlags(vk::ImageUsageFlagBits::eSampled)
                                       .SetFormat(vk::Format::eR8G8B8A8Unorm)
                                       .SetMips(std::floor(std::log2(std::max(width, height))));

                                   stbi_image_free(stbiData);
                               } },
                           buffer.data);
                   },
               },
        gltfImage.data);

    return cpuImage;
}

struct StagingAnimationChannels
{
    std::vector<Animation> animations;

    struct IndexChannel
    {
        std::vector<TransformAnimationSpline> animationChannels;
        std::vector<uint32_t> nodeIndices;
    };
    std::vector<IndexChannel> indexChannels;
};

StagingAnimationChannels LoadAnimations(const fastgltf::Asset& gltf)
{
    ZoneScopedN("Animation Loading");

    StagingAnimationChannels stagingAnimationChannels {};

    for (const auto& gltfAnimation : gltf.animations)
    {
        auto& animation = stagingAnimationChannels.animations.emplace_back();
        auto& indexChannels = stagingAnimationChannels.indexChannels.emplace_back();

        animation.name = gltfAnimation.name;

        for (const auto& channel : gltfAnimation.channels)
        {
            // If there is no node, the channel is invalid.
            if (!channel.nodeIndex.has_value())
            {
                continue;
            }

            TransformAnimationSpline* spline { nullptr };
            // Try and find whether we already created an animation channel
            const auto it = std::find(indexChannels.nodeIndices.begin(), indexChannels.nodeIndices.end(), channel.nodeIndex.value());

            // If not create it
            if (it == indexChannels.nodeIndices.end())
            {
                indexChannels.nodeIndices.emplace_back(channel.nodeIndex.value());
                spline = &indexChannels.animationChannels.emplace_back();
            }
            else // Else reuse it
            {
                spline = &indexChannels.animationChannels[std::distance(indexChannels.nodeIndices.begin(), it)];
            }

            const auto& sampler = gltfAnimation.samplers[channel.samplerIndex];

            std::span<const float> timestamps;
            {
                const auto& accessor = gltf.accessors[sampler.inputAccessor];
                const auto& bufferView = gltf.bufferViews[accessor.bufferViewIndex.value()];
                const auto& buffer = gltf.buffers[bufferView.bufferIndex];
                assert(!accessor.sparse.has_value() && "No support for sparse accesses");
                assert(!bufferView.byteStride.has_value() && "No support for byte stride view");

                const std::byte* data = std::get<fastgltf::sources::Array>(buffer.data).bytes.data() + bufferView.byteOffset + accessor.byteOffset;
                timestamps = std::span<const float> { reinterpret_cast<const float*>(data), accessor.count };

                if (animation.duration < timestamps.back())
                {
                    animation.duration = timestamps.back();
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
                    // Parse quaternions from xyzw -> wxyz.
                    for (size_t i = 0; i < spline->rotation.value().values.size(); ++i)
                    {
                        math::QuatXYZWtoWXYZ(spline->rotation.value().values[i]);
                    }
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
    }

    return stagingAnimationChannels;
}

CPUMaterial ProcessMaterial(const fastgltf::Material& gltfMaterial, const std::vector<fastgltf::Texture>& gltfTextures)
{
    auto MapTextureIndexToImageIndex = [](uint32_t textureIndex, const std::vector<fastgltf::Texture>& gltfTextures) -> uint32_t
    {
        return gltfTextures[textureIndex].imageIndex.value();
    };

    CPUMaterial material {};

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

uint32_t RecurseHierarchy(const fastgltf::Node& gltfNode,
    uint32_t gltfNodeIndex,
    CPUModel& model,
    const fastgltf::Asset& gltf,
    const StagingAnimationChannels& stagingAnimationChannels,
    const std::unordered_multimap<uint32_t, std::pair<MeshType, uint32_t>>& meshLUT, // Used for looking up gltf mesh to engine mesh.
    std::unordered_map<uint32_t, uint32_t>& nodeLUT) // Will be populated with gltf node to engine node.
{
    model.hierarchy.nodes.emplace_back(Node {});
    uint32_t nodeIndex = model.hierarchy.nodes.size() - 1;

    nodeLUT[gltfNodeIndex] = nodeIndex;

    if (gltfNode.meshIndex.has_value())
    {
        // Add meshes as children, since glTF assumes 1 node -> 1 mesh -> >1 primitives
        // But now we want 1 node -> mesh (mesh == primitive)
        auto range = meshLUT.equal_range(gltfNode.meshIndex.value());
        for (auto it = range.first; it != range.second; ++it)
        {
            auto node = Node { "mesh node", glm::identity<glm::mat4>() };
            node.mesh = { it->second.first, it->second.second };

            model.hierarchy.nodes.emplace_back(node);
            model.hierarchy.nodes[nodeIndex].childrenIndices.emplace_back(static_cast<uint32_t>(model.hierarchy.nodes.size() - 1));
        }
    }

    // Set transform and name.
    fastgltf::math::fmat4x4 gltfTransform = fastgltf::getTransformMatrix(gltfNode);
    model.hierarchy.nodes[nodeIndex].transform = detail::ToMat4(gltfTransform);
    model.hierarchy.nodes[nodeIndex].name = gltfNode.name;

    // If we have an animation channel that should be used on this node, we apply it.
    for (size_t i = 0; i < stagingAnimationChannels.animations.size(); ++i)
    {
        for (size_t j = 0; j < stagingAnimationChannels.indexChannels[i].nodeIndices.size(); j++)
        {
            if (stagingAnimationChannels.indexChannels[i].nodeIndices[j] == gltfNodeIndex)
            {
                AnimationChannelComponent animationChannelComponent {};
                model.hierarchy.nodes[nodeIndex].animationSplines[i] = stagingAnimationChannels.indexChannels[i].animationChannels[j];
                break;
            }
        }
    }

    // Check all the gltf skins for skinning data that might be applied to this node.
    for (size_t i = 0; i < gltf.skins.size(); ++i)
    {
        const auto& skin = gltf.skins[i];

        // Find whether this node is part of a joint in the skin.
        auto it = std::find(skin.joints.begin(), skin.joints.end(), gltfNodeIndex);
        if (it != skin.joints.end())
        {
            // Get the inverse bind matrix for this joint.
            fastgltf::math::fmat4x4 inverseBindMatrix = fastgltf::getAccessorElement<fastgltf::math::fmat4x4>(gltf, gltf.accessors[skin.inverseBindMatrices.value()], std::distance(skin.joints.begin(), it));

            uint32_t jointIndex = std::distance(skin.joints.begin(), it);
            model.hierarchy.nodes[nodeIndex].joint = NodeJointData {
                jointIndex,
                *reinterpret_cast<glm::mat4x4*>(&inverseBindMatrix)
            };
        }
    }

    if (gltfNode.lightIndex.has_value())
    {
        const auto& light = gltf.lights[gltfNode.lightIndex.value()];

        NodeLightData lightData;
        lightData.color = detail::ToVec3(light.color);
        lightData.range = light.range.has_value() ? light.range.value() * 1000 : NodeLightData::DEFAULT_LIGHT_RANGE;
        lightData.intensity = light.intensity;
        switch (light.type)
        {
        case fastgltf::LightType::Directional:
            lightData.type = NodeLightType::Directional;
            break;
        case fastgltf::LightType::Point:
            lightData.type = NodeLightType::Point;
            break;
        case fastgltf::LightType::Spot:
            lightData.type = NodeLightType::Spot;
            break;
        default:
            lightData.type = NodeLightType::Point;
            break;
        }

        model.hierarchy.nodes[nodeIndex].light = lightData;
    }

    for (size_t childNodeIndex : gltfNode.children)
    {
        uint32_t index = RecurseHierarchy(gltf.nodes[childNodeIndex], childNodeIndex, model, gltf, stagingAnimationChannels, meshLUT, nodeLUT);
        model.hierarchy.nodes[nodeIndex].childrenIndices.emplace_back(index);
    }

    return nodeIndex;
}

CPUModel ProcessModel(const fastgltf::Asset& gltf, const std::string_view name)
{
    ZoneScoped;

    std::string zone = std::string(name) + " data processing";
    ZoneName(zone.c_str(), 128);

    CPUModel model {};
    model.name = name;

    // Extract texture data
    std::vector<std::vector<std::byte>> textureData(gltf.images.size());

    for (size_t i = 0; i < gltf.images.size(); ++i)
    {
        model.textures.emplace_back(ProcessImage(gltf.images[i], gltf, textureData[i], name));
    }

    // Extract material data
    for (auto& gltfMaterial : gltf.materials)
    {
        const CPUMaterial material = ProcessMaterial(gltfMaterial, gltf.textures);
        model.materials.emplace_back(material);
    }

    // Tracks gltf mesh index to our engine mesh index.
    std::unordered_multimap<uint32_t, std::pair<MeshType, uint32_t>> meshLUT {};

    // Extract mesh data
    {
        ZoneScopedN("Mesh Loading");

        uint32_t counter { 0 };
        for (auto& gltfMesh : gltf.meshes)
        {
            for (const auto& gltfPrimitive : gltfMesh.primitives)
            {
                if (gltf.skins.size() > 0 && gltfPrimitive.findAttribute("WEIGHTS_0") != gltfPrimitive.attributes.cend() && gltfPrimitive.findAttribute("JOINTS_0") != gltfPrimitive.attributes.cend())
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
    }

    StagingAnimationChannels animations {};
    if (!gltf.animations.empty())
    {
        animations = LoadAnimations(gltf);
        model.animations = animations.animations;
    }

    model.hierarchy.nodes.emplace_back(Node {});
    uint32_t baseNodeIndex = model.hierarchy.nodes.size() - 1;
    model.hierarchy.nodes[baseNodeIndex].name = name;

    model.hierarchy.root = model.hierarchy.nodes.size() - 1;

    std::unordered_map<uint32_t, uint32_t> nodeLUT {};

    for (size_t i = 0; i < gltf.scenes[0].nodeIndices.size(); ++i)
    {
        uint32_t index = gltf.scenes[0].nodeIndices[i];
        const auto& gltfNode { gltf.nodes[index] };

        uint32_t result = RecurseHierarchy(gltfNode, index, model, gltf, animations, meshLUT, nodeLUT);
        model.hierarchy.nodes[baseNodeIndex].childrenIndices.emplace_back(result);
    }

    for (size_t i = 0; i < gltf.skins.size(); ++i)
    {
        const auto& skin = gltf.skins[i];

        std::function<bool(Node&, uint32_t)> traverse = [&model, &skin, &nodeLUT, &traverse](Node& node, uint32_t nodeIndex)
        {
            auto it = std::find_if(skin.joints.begin(), skin.joints.end(), [&nodeLUT, nodeIndex](uint32_t index)
                { return nodeLUT[index] == nodeIndex; });

            if (it != skin.joints.end())
            {
                return true;
            }

            std::vector<uint32_t> baseJoints {};
            for (size_t i = 0; i < node.childrenIndices.size(); ++i)
            {
                if (traverse(model.hierarchy.nodes[node.childrenIndices[i]], node.childrenIndices[i]))
                {
                    baseJoints.emplace_back(node.childrenIndices[i]);
                }
            }

            if (baseJoints.size() > 0)
            {
                Node& skeletonNode = model.hierarchy.nodes.emplace_back();
                model.hierarchy.skeletonRoot = model.hierarchy.nodes.size() - 1;
                skeletonNode.name = "Skeleton Node";
                std::copy(baseJoints.begin(), baseJoints.end(), std::back_inserter(skeletonNode.childrenIndices));
                std::erase_if(node.childrenIndices, [&baseJoints](auto index)
                    { return std::find(baseJoints.begin(), baseJoints.end(), index) != baseJoints.end(); });

                return false;
            }

            return false;
        };

        traverse(model.hierarchy.nodes[model.hierarchy.root], model.hierarchy.root);
    }

    // After instantiating hierarchy, we do another pass to apply missing child references.
    // In this case we match all the skeleton indices.
    // Note, that we have to account for expanding the primitives into their own nodes.
    for (size_t i = 0; i < gltf.nodes.size(); ++i)
    {
        const auto& gltfNode = gltf.nodes[i];
        auto& node = model.hierarchy.nodes[nodeLUT[i]];

        // Skins and meshes come together, we can assume here there is also a skinned mesh.
        if (gltfNode.skinIndex.has_value())
        {
            // Iterate over all the children of the matched node, since we expanded the primitives.
            for (auto childIndex : node.childrenIndices)
            {
                // Apply the skeleton node using the node LUT.
                model.hierarchy.nodes[childIndex].skeletonNode = model.hierarchy.skeletonRoot;
            }
        }
    }

    // Moves the animation from the model root to the skeleton root
    if (model.hierarchy.skeletonRoot.has_value())
    {
        auto& firstChild = model.hierarchy.nodes[model.hierarchy.nodes[model.hierarchy.root].childrenIndices[0]];
        if (!firstChild.animationSplines.empty())
        {
            model.hierarchy.nodes[model.hierarchy.skeletonRoot.value()].animationSplines = firstChild.animationSplines;
            firstChild.animationSplines = {};
            firstChild.transform = {};
        }
    }

    return model;
}

CPUModel ModelLoading::LoadGLTFFast(ThreadPool& scheduler, std::string_view path, bool genCollision)
{
    size_t offset = path.find_last_of('/') + 1;
    std::string_view name = path.substr(offset, path.find_last_of('.') - offset);

    auto gltf = detail::LoadFastGLTFAsset(path);

    CPUModel model {};
    model.name = name;

    std::vector<std::future<CPUImage>> imageLoadResults {};

    for (const auto& image : gltf.images)
    {
        auto future = scheduler.QueueWork([&gltf, &image]()
            { return detail::ProcessImage(gltf, image); });
        imageLoadResults.emplace_back(std::move(future));
    }

    // Extract material data
    for (auto& gltfMaterial : gltf.materials)
    {
        const CPUMaterial material = ProcessMaterial(gltfMaterial, gltf.textures);
        model.materials.emplace_back(material);
    }

    //
    // Tracks gltf mesh index to our engine mesh index.
    std::unordered_multimap<uint32_t, std::pair<MeshType, uint32_t>> meshLUT {};

    // Extract mesh data
    {
        ZoneScopedN("Mesh Loading");

        uint32_t counter { 0 };
        for (auto& gltfMesh : gltf.meshes)
        {
            for (const auto& gltfPrimitive : gltfMesh.primitives)
            {
                if (gltf.skins.size() > 0 && gltfPrimitive.findAttribute("WEIGHTS_0") != gltfPrimitive.attributes.cend() && gltfPrimitive.findAttribute("JOINTS_0") != gltfPrimitive.attributes.cend())
                {
                    CPUMesh<SkinnedVertex> primitive = ProcessPrimitive<SkinnedVertex>(gltfPrimitive, gltf);
                    model.skinnedMeshes.emplace_back(primitive);

                    meshLUT.insert({ counter, std::pair(MeshType::eSKINNED, model.skinnedMeshes.size() - 1) });
                }
                else
                {
                    CPUMesh<Vertex> primitive = ProcessPrimitive<Vertex>(gltfPrimitive, gltf);

                    if (genCollision)
                    {
                        model.colliders.emplace_back(detail::ProcessMeshIntoCollider(primitive));
                    }

                    model.meshes.emplace_back(primitive);
                    meshLUT.insert({ counter, std::pair(MeshType::eSTATIC, model.meshes.size() - 1) });
                }
            }
            ++counter;
        }
    }

    StagingAnimationChannels animations {};
    if (!gltf.animations.empty())
    {
        animations = LoadAnimations(gltf);
        model.animations = animations.animations;
    }

    model.hierarchy.nodes.emplace_back(Node {});
    uint32_t baseNodeIndex = model.hierarchy.nodes.size() - 1;
    model.hierarchy.nodes[baseNodeIndex].name = name;

    model.hierarchy.root = model.hierarchy.nodes.size() - 1;

    std::unordered_map<uint32_t, uint32_t> nodeLUT {};

    for (size_t i = 0; i < gltf.scenes[0].nodeIndices.size(); ++i)
    {
        uint32_t index = gltf.scenes[0].nodeIndices[i];
        const auto& gltfNode { gltf.nodes[index] };

        uint32_t result = RecurseHierarchy(gltfNode, index, model, gltf, animations, meshLUT, nodeLUT);
        model.hierarchy.nodes[baseNodeIndex].childrenIndices.emplace_back(result);
    }

    for (size_t i = 0; i < gltf.skins.size(); ++i)
    {
        const auto& skin = gltf.skins[i];

        std::function<bool(uint32_t)> traverse = [&model, &skin, &nodeLUT, &traverse](uint32_t nodeIndex)
        {
            auto it = std::find_if(skin.joints.begin(), skin.joints.end(), [&nodeLUT, nodeIndex](uint32_t index)
                { return nodeLUT[index] == nodeIndex; });

            if (it != skin.joints.end())
            {
                return true;
            }

            std::vector<uint32_t> baseJoints {};
            for (size_t i = 0; i < model.hierarchy.nodes[nodeIndex].childrenIndices.size(); ++i)
            {
                if (traverse(model.hierarchy.nodes[nodeIndex].childrenIndices[i]))
                {
                    baseJoints.emplace_back(model.hierarchy.nodes[nodeIndex].childrenIndices[i]);
                }
            }

            if (baseJoints.size() > 0)
            {
                Node& skeletonNode = model.hierarchy.nodes.emplace_back();
                model.hierarchy.skeletonRoot = model.hierarchy.nodes.size() - 1;
                skeletonNode.name = "Skeleton Node";

                std::copy(baseJoints.begin(), baseJoints.end(), std::back_inserter(skeletonNode.childrenIndices));
                std::erase_if(model.hierarchy.nodes[nodeIndex].childrenIndices, [&baseJoints](auto index)
                    { return std::find(baseJoints.begin(), baseJoints.end(), index) != baseJoints.end(); });

                return false;
            }

            return false;
        };

        traverse(model.hierarchy.root);
    }

    // After instantiating hierarchy, we do another pass to apply missing child references.
    // In this case we match all the skeleton indices.
    // Note, that we have to account for expanding the primitives into their own nodes.
    for (size_t i = 0; i < gltf.nodes.size(); ++i)
    {
        const auto& gltfNode = gltf.nodes[i];
        auto& node = model.hierarchy.nodes[nodeLUT[i]];

        // Skins and meshes come together, we can assume here there is also a skinned mesh.
        if (gltfNode.skinIndex.has_value())
        {
            // Iterate over all the children of the matched node, since we expanded the primitives.
            for (auto childIndex : node.childrenIndices)
            {
                // Apply the skeleton node using the node LUT.
                model.hierarchy.nodes[childIndex].skeletonNode = model.hierarchy.skeletonRoot;
            }
        }
    }

    for (auto& image : imageLoadResults)
    {
        model.textures.emplace_back(image.get());
    }

    // Moves the animation from the model root to the skeleton root
    if (model.hierarchy.skeletonRoot.has_value())
    {
        auto& firstChild = model.hierarchy.nodes[model.hierarchy.nodes[model.hierarchy.root].childrenIndices[0]];
        if (!firstChild.animationSplines.empty())
        {
            model.hierarchy.nodes[model.hierarchy.skeletonRoot.value()].animationSplines = firstChild.animationSplines;
            firstChild.animationSplines = {};
            firstChild.transform = glm::identity<glm::mat4>();
        }
    }

    return model;
}