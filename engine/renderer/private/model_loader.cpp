#include "model_loader.hpp"

#include "batch_buffer.hpp"
#include "ecs_module.hpp"
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

#include <glm/gtc/type_ptr.hpp>
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

void CalculateTangents(CPUMesh::Primitive& stagingPrimitive)
{
    uint32_t triangleCount = stagingPrimitive.indices.size() > 0 ? stagingPrimitive.indices.size() / 3 : stagingPrimitive.vertices.size() / 3;
    for (size_t i = 0; i < triangleCount; ++i)
    {
        std::array<Vertex*, 3> triangle = {};
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
}

CPUMesh::Primitive ProcessPrimitive(const fastgltf::Primitive& gltfPrimitive, const fastgltf::Asset& gltf)
{
    CPUMesh::Primitive primitive {};
    primitive.boundingBox.min = glm::vec3 { std::numeric_limits<float>::max() };
    primitive.boundingBox.max = glm::vec3 { std::numeric_limits<float>::lowest() };
    assert(MapGltfTopology(gltfPrimitive.type) == vk::PrimitiveTopology::eTriangleList && "Only triangle list topology is supported!");
    if (gltfPrimitive.materialIndex.has_value())
        primitive.materialIndex = gltfPrimitive.materialIndex.value();

    bool verticesReserved = false;
    bool tangentFound = false;
    bool texCoordFound = false;
    float squaredBoundingRadius = 0.0f;

    for (auto& attribute : gltfPrimitive.attributes)
    {
        auto& accessor = gltf.accessors[attribute.accessorIndex];
        if (!accessor.bufferViewIndex.has_value())
            throw std::runtime_error("Failed retrieving buffer view index from accessor!");
        auto& bufferView = gltf.bufferViews[accessor.bufferViewIndex.value()];
        auto& buffer = gltf.buffers[bufferView.bufferIndex];
        auto& bufferBytes = std::get<fastgltf::sources::Array>(buffer.data);

        const std::byte* attributeBufferStart = bufferBytes.bytes.data() + bufferView.byteOffset + accessor.byteOffset;

        // Make sure the mesh primitive has enough space allocated.
        if (!verticesReserved)
        {
            primitive.vertices = std::vector<Vertex>(accessor.count);
            verticesReserved = true;
        }

        std::uint32_t offset;
        if (attribute.name == "POSITION")
            offset = offsetof(Vertex, position);
        else if (attribute.name == "NORMAL")
            offset = offsetof(Vertex, normal);
        else if (attribute.name == "TANGENT")
        {
            offset = offsetof(Vertex, tangent);
            tangentFound = true;
        }
        else if (attribute.name == "TEXCOORD_0")
        {
            offset = offsetof(Vertex, texCoord);
            texCoordFound = true;
        }
        else
        {
            continue;
        }

        for (size_t i = 0; i < accessor.count; ++i)
        {
            const std::byte* element;
            if (bufferView.byteStride.has_value())
                element = attributeBufferStart + i * bufferView.byteStride.value();
            else
                element = attributeBufferStart + i * fastgltf::getElementByteSize(accessor.type, accessor.componentType);

            std::byte* writeTarget = reinterpret_cast<std::byte*>(&primitive.vertices[i]) + offset;
            std::memcpy(writeTarget, element, fastgltf::getElementByteSize(accessor.type, accessor.componentType));

            if (attribute.name == "POSITION")
            {

                const glm::vec3* position = reinterpret_cast<const glm::vec3*>(element);

                // warning! this performs component wise min/max
                primitive.boundingBox.min = glm::min(primitive.boundingBox.min, *position);
                primitive.boundingBox.max = glm::max(primitive.boundingBox.max, *position);

                float squaredLength = position->x * position->x + position->y * position->y + position->z * position->z;
                if (squaredLength > squaredBoundingRadius)
                    squaredBoundingRadius = squaredLength;
            }
        }
    }

    primitive.boundingRadius = glm::sqrt(squaredBoundingRadius);

    if (gltfPrimitive.indicesAccessor.has_value())
    {
        auto& accessor = gltf.accessors[gltfPrimitive.indicesAccessor.value()];
        if (!accessor.bufferViewIndex.has_value())
            throw std::runtime_error("Failed retrieving buffer view index from accessor!");
        auto& bufferView = gltf.bufferViews[accessor.bufferViewIndex.value()];
        auto& buffer = gltf.buffers[bufferView.bufferIndex];
        auto& bufferBytes = std::get<fastgltf::sources::Array>(buffer.data);

        primitive.indices = std::vector<uint32_t>(accessor.count);

        const std::byte* attributeBufferStart = bufferBytes.bytes.data() + bufferView.byteOffset + accessor.byteOffset;

        if (accessor.componentType == fastgltf::ComponentType::UnsignedInt && (!bufferView.byteStride.has_value() || bufferView.byteStride.value() == 0))
        {
            std::memcpy(primitive.indices.data(), attributeBufferStart, primitive.indices.size() * sizeof(uint32_t));
        }
        else
        {
            uint32_t gltfIndexTypeSize = fastgltf::getComponentByteSize(accessor.componentType);
            for (size_t i = 0; i < accessor.count; ++i)
            {
                const std::byte* element = attributeBufferStart + i * gltfIndexTypeSize + (bufferView.byteStride.has_value() ? bufferView.byteStride.value() : 0);
                uint32_t* indexPtr = primitive.indices.data() + i;
                std::memcpy(indexPtr, element, gltfIndexTypeSize);
            }
        }
    }

    if (!tangentFound && texCoordFound)
        CalculateTangents(primitive);

    return primitive;
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

Hierarchy::Node RecurseHierarchy(const fastgltf::Node& gltfNode, CPUModel& model, const fastgltf::Asset& gltf)
{
    Hierarchy::Node node {};

    if (gltfNode.meshIndex.has_value())
    {
        node.meshIndex = gltfNode.meshIndex.value();
    }
    else
    {
        node.meshIndex = std::nullopt;
    }

    fastgltf::math::fmat4x4 gltfTransform = fastgltf::getTransformMatrix(gltfNode);
    node.transform = detail::ToMat4(gltfTransform);
    node.name = gltfNode.name;

    for (size_t i : gltfNode.children)
    {
        node.children.emplace_back(RecurseHierarchy(gltf.nodes[i], model, gltf));
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

    // Extract mesh data
    for (auto& gltfMesh : gltf.meshes)
    {
        CPUMesh mesh {};

        for (const auto& gltfPrimitive : gltfMesh.primitives)
        {
            CPUMesh::Primitive primitive = ProcessPrimitive(gltfPrimitive, gltf);
            mesh.primitives.emplace_back(primitive);
        }

        model.meshes.emplace_back(mesh);
    }

    Hierarchy::Node baseNode;
    baseNode.name = name;

    for (size_t i = 0; i < gltf.scenes[0].nodeIndices.size(); ++i)
    {
        const auto& gltfNode { gltf.nodes[gltf.scenes[0].nodeIndices[i]] };
        baseNode.children.emplace_back(RecurseHierarchy(gltfNode, model, gltf));
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
