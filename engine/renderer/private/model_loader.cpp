#include "model_loader.hpp"
#include "batch_buffer.hpp"
#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "log.hpp"
#include "single_time_commands.hpp"
#include "stb_image.h"
#include "timers.hpp"
#include "vulkan_helper.hpp"

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

ModelLoader::ModelLoader(const VulkanContext& brain, std::shared_ptr<const ECS> ecs)
    : _brain(brain)
    , _ecs(ecs)
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

Model ModelLoader::Load(std::string_view path, BatchBuffer& batchBuffer, LoadMode loadMode)
{
    Stopwatch stopwatch;
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

    bblog::info("Loaded model: {} in {} ms", path, stopwatch.GetElapsed().count());

    return LoadModel(gltf, batchBuffer, name, loadMode);
}

StagingMesh::Primitive ModelLoader::ProcessPrimitive(const fastgltf::Primitive& gltfPrimitive, const fastgltf::Asset& gltf)
{
    StagingMesh::Primitive stagingPrimitive {};

    assert(MapGltfTopology(gltfPrimitive.type) == vk::PrimitiveTopology::eTriangleList && "Only triangle list topology is supported!");
    if (gltfPrimitive.materialIndex.has_value())
        stagingPrimitive.materialIndex = gltfPrimitive.materialIndex.value();

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
            stagingPrimitive.vertices = std::vector<Vertex>(accessor.count);
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

            std::byte* writeTarget = reinterpret_cast<std::byte*>(&stagingPrimitive.vertices[i]) + offset;
            std::memcpy(writeTarget, element, fastgltf::getElementByteSize(accessor.type, accessor.componentType));

            if (attribute.name == "POSITION")
            {
                const glm::vec3* position = reinterpret_cast<const glm::vec3*>(element);
                float squaredLength = position->x * position->x + position->y * position->y + position->z * position->z;

                if (squaredLength > squaredBoundingRadius)
                    squaredBoundingRadius = squaredLength;
            }
        }
    }

    stagingPrimitive.boundingRadius = glm::sqrt(squaredBoundingRadius);

    if (gltfPrimitive.indicesAccessor.has_value())
    {
        auto& accessor = gltf.accessors[gltfPrimitive.indicesAccessor.value()];
        if (!accessor.bufferViewIndex.has_value())
            throw std::runtime_error("Failed retrieving buffer view index from accessor!");
        auto& bufferView = gltf.bufferViews[accessor.bufferViewIndex.value()];
        auto& buffer = gltf.buffers[bufferView.bufferIndex];
        auto& bufferBytes = std::get<fastgltf::sources::Array>(buffer.data);

        stagingPrimitive.indices = std::vector<uint32_t>(accessor.count);

        const std::byte* attributeBufferStart = bufferBytes.bytes.data() + bufferView.byteOffset + accessor.byteOffset;

        if (accessor.componentType == fastgltf::ComponentType::UnsignedInt && (!bufferView.byteStride.has_value() || bufferView.byteStride.value() == 0))
        {
            std::memcpy(stagingPrimitive.indices.data(), attributeBufferStart, stagingPrimitive.indices.size() * sizeof(uint32_t));
        }
        else
        {
            uint32_t gltfIndexTypeSize = fastgltf::getComponentByteSize(accessor.componentType);
            for (size_t i = 0; i < accessor.count; ++i)
            {
                const std::byte* element = attributeBufferStart + i * gltfIndexTypeSize + (bufferView.byteStride.has_value() ? bufferView.byteStride.value() : 0);
                uint32_t* indexPtr = stagingPrimitive.indices.data() + i;
                std::memcpy(indexPtr, element, gltfIndexTypeSize);
            }
        }
    }

    if (!tangentFound && texCoordFound)
        CalculateTangents(stagingPrimitive);

    return stagingPrimitive;
}

ImageCreation ModelLoader::ProcessImage(const fastgltf::Image& gltfImage, const fastgltf::Asset& gltf, std::vector<std::byte>& data,
    std::string_view name)
{
    ImageCreation imageCreation {};

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

uint32_t ModelLoader::MapTextureIndexToImageIndex(uint32_t textureIndex, const fastgltf::Asset& gltf)
{
    return gltf.textures[textureIndex].imageIndex.value();
}

void ModelLoader::CalculateTangents(StagingMesh::Primitive& stagingPrimitive)
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

glm::vec4 ModelLoader::CalculateTangent(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2,
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

Model ModelLoader::LoadModel(const fastgltf::Asset& gltf, BatchBuffer& batchBuffer, const std::string_view name, LoadMode loadMode)
{
    Model model {};

    SingleTimeCommands commandBuffer { _brain };

    // Load textures
    std::vector<std::vector<std::byte>> textureData(gltf.images.size());

    for (size_t i = 0; i < gltf.images.size(); ++i)
    {
        ImageCreation imageCreation = ProcessImage(gltf.images[i], gltf, textureData[i], name);
        model.resources.textures.emplace_back(_brain.GetImageResourceManager().Create(imageCreation));
    }

    // Load materials
    for (auto& material : gltf.materials)
    {
        MaterialCreation materialCreation = ProcessMaterial(material, model.resources.textures, gltf);
        model.resources.materials.emplace_back(_brain.GetMaterialResourceManager().Create(materialCreation));
    }

    // Load meshes
    for (auto& gltfMesh : gltf.meshes)
    {
        Mesh mesh {};

        for (const auto& gltfPrimitive : gltfMesh.primitives)
        {
            StagingMesh::Primitive stagingPrimitive = ProcessPrimitive(gltfPrimitive, gltf);
            Mesh::Primitive primitive = LoadPrimitive(stagingPrimitive, commandBuffer, batchBuffer,
                gltfPrimitive.materialIndex.has_value() ? model.resources.materials[gltfPrimitive.materialIndex.value()] : ResourceHandle<Material>::Invalid());
            mesh.primitives.emplace_back(primitive);
        }

        auto handle = _brain.GetMeshResourceManager().Create(mesh);

        model.resources.meshes.emplace_back(handle);
    }

    switch (loadMode)
    {
    case LoadMode::eFlat:
        for (size_t i = 0; i < gltf.scenes[0].nodeIndices.size(); ++i)
        {
            const auto& gltfNode { gltf.nodes[gltf.scenes[0].nodeIndices[i]] };
            Hierarchy::Node& baseNode = model.hierarchy.baseNodes.emplace_back(Hierarchy::Node {});
            baseNode.name = gltfNode.name;

            RecurseHierarchy(gltfNode, model, gltf, baseNode);
        }
        break;
    case LoadMode::eHierarchical:
        Hierarchy::Node& baseNode = model.hierarchy.baseNodes.emplace_back(Hierarchy::Node {});
        baseNode.name = gltf.scenes[0].name;

        for (size_t i = 0; i < gltf.scenes[0].nodeIndices.size(); ++i)
        {
            const auto& gltfNode { gltf.nodes[gltf.scenes[0].nodeIndices[i]] };
            RecurseHierarchy(gltfNode, model, gltf, baseNode);
        }
        break;
    }

    commandBuffer.Submit();

    return model;
}

Mesh::Primitive ModelLoader::LoadPrimitive(const StagingMesh::Primitive& stagingPrimitive, SingleTimeCommands& commandBuffer, BatchBuffer& batchBuffer,
    ResourceHandle<Material> material)
{
    Mesh::Primitive primitive {};
    primitive.material = _brain.GetMaterialResourceManager().IsValid(material) ? material : _defaultMaterial;
    primitive.count = stagingPrimitive.indices.size();
    primitive.vertexOffset = batchBuffer.AppendVertices(stagingPrimitive.vertices, commandBuffer);
    primitive.indexOffset = batchBuffer.AppendIndices(stagingPrimitive.indices, commandBuffer);
    primitive.boundingRadius = stagingPrimitive.boundingRadius;

    return primitive;
}

void ModelLoader::RecurseHierarchy(const fastgltf::Node& gltfNode, Model& model, const fastgltf::Asset& gltf, Hierarchy::Node& parent)
{
    Hierarchy::Node& node { parent.children.emplace_back() };

    if (gltfNode.meshIndex.has_value())
    {
        node.mesh = model.resources.meshes[gltfNode.meshIndex.value()];
    }
    else
    {
        node.mesh = ResourceHandle<Mesh>::Invalid();
    }

    fastgltf::math::fmat4x4 gltfTransform = fastgltf::getTransformMatrix(gltfNode);
    node.transform = detail::ToMat4(gltfTransform);
    node.name = gltfNode.name;

    for (size_t i : gltfNode.children)
    {
        RecurseHierarchy(gltf.nodes[i], model, gltf, node);
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

ResourceHandle<Mesh> ModelLoader::LoadMesh(const StagingMesh::Primitive& stagingPrimitive, SingleTimeCommands& commandBuffer, BatchBuffer& batchBuffer, ResourceHandle<Material> material)
{
    auto primitive = LoadPrimitive(stagingPrimitive, commandBuffer, batchBuffer, material);
    Mesh mesh;
    mesh.primitives.emplace_back(primitive);

    return _brain.GetMeshResourceManager().Create(mesh);
}
