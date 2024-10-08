#include "model_loader.hpp"

#include "ECS.hpp"
#include "spdlog/spdlog.h"
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include "stb_image.h"
#include "vulkan_helper.hpp"
#include "single_time_commands.hpp"
#include "batch_buffer.hpp"
#include "glm/gtc/type_ptr.hpp"

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

ModelLoader::ModelLoader(const VulkanBrain& brain)
    : _brain(brain)
{

    _sampler = util::CreateSampler(_brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat,
        vk::SamplerMipmapMode::eLinear, static_cast<uint32_t>(floor(log2(2048))));

    auto data = std::vector<std::byte>(2 * 2 * sizeof(std::byte) * 4);
    ImageCreation defaultImageCreation {};
    defaultImageCreation.SetName("Default image").SetData(data.data()).SetSize(2, 2).SetFlags(vk::ImageUsageFlagBits::eSampled).SetFormat(vk::Format::eR8G8B8A8Unorm);

    ResourceHandle<Image> defaultImage = _brain.GetImageResourceManager().Create(defaultImageCreation);

    MaterialCreation defaultMaterialCreationInfo {};
    defaultMaterialCreationInfo.albedoMap = defaultImage;
    _defaultMaterial = _brain.GetMaterialResourceManager().Create(defaultMaterialCreationInfo);
}

ModelLoader::~ModelLoader()
{
    const Material* defaultMaterial = _brain.GetMaterialResourceManager().Access(_defaultMaterial);
    _brain.GetImageResourceManager().Destroy(defaultMaterial->albedoMap);
    _brain.GetMaterialResourceManager().Destroy(_defaultMaterial);
}

ModelHandle ModelLoader::Load(std::string_view path, BatchBuffer& batchBuffer)
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
        spdlog::warn("GLTF contains more than one scene, but we only load one scene!");

    spdlog::info("Loaded model: {}", path);

    return LoadModel(gltf, batchBuffer, name);
}

MeshPrimitive ModelLoader::ProcessPrimitive(const fastgltf::Primitive& gltfPrimitive, const fastgltf::Asset& gltf)
{
    MeshPrimitive primitive {};

    assert(MapGltfTopology(gltfPrimitive.type) == vk::PrimitiveTopology::eTriangleList && "Only triangle list topology is supported!");
    if (gltfPrimitive.materialIndex.has_value())
        primitive.materialIndex = gltfPrimitive.materialIndex.value();

    bool verticesReserved = false;
    bool tangentFound = false;
    bool texCoordFound = false;

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
            continue;

        for (size_t i = 0; i < accessor.count; ++i)
        {
            const std::byte* element;
            if (bufferView.byteStride.has_value())
                element = attributeBufferStart + i * bufferView.byteStride.value();
            else
                element = attributeBufferStart + i * fastgltf::getElementByteSize(accessor.type, accessor.componentType);

            std::byte* writeTarget = reinterpret_cast<std::byte*>(&primitive.vertices[i]) + offset;
            std::memcpy(writeTarget, element, fastgltf::getElementByteSize(accessor.type, accessor.componentType));
        }
    }

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

ImageCreation
ModelLoader::ProcessImage(const fastgltf::Image& gltfImage, const fastgltf::Asset& gltf, std::vector<std::byte>& data,
    std::string_view name)
{
    ImageCreation imageCreation {};

    std::visit(fastgltf::visitor {
                   [](auto& arg) {},
                   [&](const fastgltf::sources::URI& filePath)
                   {
                       assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
                       assert(filePath.uri.isLocalPath()); // We're only capable of loading local files.
                       int32_t width, height, nrChannels;

                       const std::string path(filePath.uri.path().begin(), filePath.uri.path().end()); // Thanks C++.
                       stbi_uc* stbiData = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                       if (!stbiData)
                           spdlog::error("Failed loading data from STBI at path: {}", path);

                       data = std::vector<std::byte>(width * height * 4);
                       std::memcpy(data.data(), reinterpret_cast<std::byte*>(stbiData), data.size());

                       imageCreation.SetName(name).SetSize(width, height).SetData(data.data()).SetFlags(vk::ImageUsageFlagBits::eSampled).SetFormat(vk::Format::eR8G8B8A8Unorm);

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

                       imageCreation.SetName(name).SetSize(width, height).SetData(data.data()).SetFlags(vk::ImageUsageFlagBits::eSampled).SetFormat(vk::Format::eR8G8B8A8Unorm);

                       stbi_image_free(stbiData);
                   },
                   [&](const fastgltf::sources::BufferView& view)
                   {
                       auto& bufferView = gltf.bufferViews[view.bufferViewIndex];
                       auto& buffer = gltf.buffers[bufferView.bufferIndex];

                       std::visit(
                           fastgltf::visitor { // We only care about VectorWithMime here, because we specify LoadExternalBuffers, meaning
                               // all buffers are already loaded into a vector.
                               [](auto& arg) {},
                               [&](const fastgltf::sources::Array& vector)
                               {
                                   int32_t width, height, nrChannels;
                                   stbi_uc* stbiData = stbi_load_from_memory(
                                       reinterpret_cast<const stbi_uc*>(vector.bytes.data() + bufferView.byteOffset),
                                       static_cast<int32_t>(bufferView.byteLength), &width, &height, &nrChannels,
                                       4);

                                   data = std::vector<std::byte>(width * height * 4);
                                   std::memcpy(data.data(), reinterpret_cast<std::byte*>(stbiData), data.size());

                                   imageCreation.SetName(name).SetSize(width, height).SetData(data.data()).SetFlags(vk::ImageUsageFlagBits::eSampled).SetFormat(vk::Format::eR8G8B8A8Unorm);

                                   stbi_image_free(stbiData);
                               } },
                           buffer.data);
                   },
               },
        gltfImage.data);

    return imageCreation;
}

MaterialCreation ModelLoader::ProcessMaterial(const fastgltf::Material& gltfMaterial, const std::vector<ResourceHandle<Image>>& modelTextures, const fastgltf::Asset& gltf)
{
    MaterialCreation material {};

    if (gltfMaterial.pbrData.baseColorTexture.has_value())
    {
        uint32_t textureIndex = MapTextureIndexToImageIndex(gltfMaterial.pbrData.baseColorTexture.value().textureIndex, gltf);
        material.albedoMap = modelTextures[textureIndex];
    }

    if (gltfMaterial.pbrData.metallicRoughnessTexture.has_value())
    {
        uint32_t textureIndex = MapTextureIndexToImageIndex(gltfMaterial.pbrData.metallicRoughnessTexture.value().textureIndex, gltf);
        material.metallicRoughnessMap = modelTextures[textureIndex];
    }

    if (gltfMaterial.normalTexture.has_value())
    {
        uint32_t textureIndex = MapTextureIndexToImageIndex(gltfMaterial.normalTexture.value().textureIndex, gltf);
        material.normalMap = modelTextures[textureIndex];
    }

    if (gltfMaterial.occlusionTexture.has_value())
    {
        uint32_t textureIndex = MapTextureIndexToImageIndex(gltfMaterial.occlusionTexture.value().textureIndex, gltf);
        material.occlusionMap = modelTextures[textureIndex];
    }

    if (gltfMaterial.emissiveTexture.has_value())
    {
        uint32_t textureIndex = MapTextureIndexToImageIndex(gltfMaterial.emissiveTexture.value().textureIndex, gltf);
        material.emissiveMap = modelTextures[textureIndex];
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

vk::PrimitiveTopology ModelLoader::MapGltfTopology(fastgltf::PrimitiveType gltfTopology)
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

vk::IndexType ModelLoader::MapIndexType(fastgltf::ComponentType componentType)
{
    switch (componentType)
    {
    case fastgltf::ComponentType::UnsignedInt:
        return vk::IndexType::eUint32;
    case fastgltf::ComponentType::UnsignedShort:
        return vk::IndexType::eUint16;
    default:
        throw std::runtime_error("Unsupported index component type!");
    }
}

uint32_t ModelLoader::MapTextureIndexToImageIndex(uint32_t textureIndex, const fastgltf::Asset& gltf)
{
    return gltf.textures[textureIndex].imageIndex.value();
}

void ModelLoader::CalculateTangents(MeshPrimitive& primitive)
{
    uint32_t triangleCount = primitive.indices.size() > 0 ? primitive.indices.size() / 3 : primitive.vertices.size() / 3;
    for (size_t i = 0; i < triangleCount; ++i)
    {
        std::array<Vertex*, 3> triangle = {};
        if (primitive.indices.size() > 0)
        {
            std::array<uint32_t, 3> indices = {};

            indices[0] = primitive.indices[(i * 3 + 0)];
            indices[1] = primitive.indices[(i * 3 + 1)];
            indices[2] = primitive.indices[(i * 3 + 2)];

            triangle = {
                &primitive.vertices[indices[0]],
                &primitive.vertices[indices[1]],
                &primitive.vertices[indices[2]]
            };
        }
        else
        {
            triangle = {
                &primitive.vertices[i * 3 + 0],
                &primitive.vertices[i * 3 + 1],
                &primitive.vertices[i * 3 + 2]
            };
        }

        glm::vec4 tangent = CalculateTangent(triangle[0]->position, triangle[1]->position, triangle[2]->position,
            triangle[0]->texCoord, triangle[1]->texCoord, triangle[2]->texCoord,
            triangle[0]->normal);

        triangle[0]->tangent += tangent;
        triangle[1]->tangent += tangent;
        triangle[2]->tangent += tangent;
    }

    for (size_t i = 0; i < primitive.vertices.size(); ++i)
    {
        glm::vec3 tangent = primitive.vertices[i].tangent;
        tangent = glm::normalize(tangent);
        primitive.vertices[i].tangent = glm::vec4 { tangent.x, tangent.y, tangent.z, primitive.vertices[i].tangent.w };
    }
}

glm::vec4
ModelLoader::CalculateTangent(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2,
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

ModelHandle
ModelLoader::LoadModel(const fastgltf::Asset& gltf, BatchBuffer& batchBuffer, const std::string_view name)
{
    SingleTimeCommands commandBuffer { _brain };

    ModelHandle modelHandle {};

    // Load textures
    std::vector<std::vector<std::byte>> textureData(gltf.images.size());

    for (size_t i = 0; i < gltf.images.size(); ++i)
    {
        ImageCreation imageCreation = ProcessImage(gltf.images[i], gltf, textureData[i], name);
        modelHandle.textures.emplace_back(_brain.GetImageResourceManager().Create(imageCreation));
    }

    // Load materials
    for (auto& material : gltf.materials)
    {
        MaterialCreation materialCreation = ProcessMaterial(material, modelHandle.textures, gltf);
        modelHandle.materials.emplace_back(_brain.GetMaterialResourceManager().Create(materialCreation));
    }

    // Load meshes
    for (auto& mesh : gltf.meshes)
    {
        MeshHandle meshHandle {};

        for (const auto& primitive : mesh.primitives)
        {
            MeshPrimitive processedPrimitive = ProcessPrimitive(primitive, gltf);
            MeshPrimitiveHandle primitivea = LoadPrimitive(processedPrimitive, commandBuffer, batchBuffer,
                primitive.materialIndex.has_value() ? modelHandle.materials[primitive.materialIndex.value()] : ResourceHandle<Material>::Invalid());
            meshHandle.primitives.emplace_back(primitivea);
        }

        modelHandle.meshes.emplace_back(std::make_shared<MeshHandle>(meshHandle));
    }

    for (size_t i = 0; i < gltf.scenes[0].nodeIndices.size(); ++i)
        RecurseHierarchy(gltf.nodes[gltf.scenes[0].nodeIndices[i]], modelHandle, gltf, glm::mat4 { 1.0f });

    commandBuffer.Submit();

    return modelHandle;
}

MeshPrimitiveHandle
ModelLoader::LoadPrimitive(const MeshPrimitive& primitive, SingleTimeCommands& commandBuffer, BatchBuffer& batchBuffer,
    ResourceHandle<Material> material)
{
    MeshPrimitiveHandle primitiveHandle {};
    primitiveHandle.material = _brain.GetMaterialResourceManager().IsValid(material) ? material : _defaultMaterial;
    primitiveHandle.count = primitive.indices.size();
    primitiveHandle.vertexOffset = batchBuffer.AppendVertices(primitive.vertices, commandBuffer);
    primitiveHandle.indexOffset = batchBuffer.AppendIndices(primitive.indices, commandBuffer);

    return primitiveHandle;
}

void ModelLoader::RecurseHierarchy(const fastgltf::Node& gltfNode, ModelHandle& modelHandle, const fastgltf::Asset& gltf,
    glm::mat4 matrix)
{
    Hierarchy::Node node {};

    if (gltfNode.meshIndex.has_value())
        node.mesh = modelHandle.meshes[gltfNode.meshIndex.value()];

    fastgltf::math::fmat4x4 transform = fastgltf::getTransformMatrix(gltfNode, detail::ToFastGLTFMat4(matrix));

    matrix = detail::ToMat4(transform);
    node.transform = matrix;
    node.name = gltfNode.name;
    if (gltfNode.meshIndex.has_value())
        modelHandle.hierarchy.allNodes.emplace_back(node);

    for (size_t i = 0; i < gltfNode.children.size(); ++i)
    {
        RecurseHierarchy(gltf.nodes[gltfNode.children[i]], modelHandle, gltf, matrix);
    }
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
