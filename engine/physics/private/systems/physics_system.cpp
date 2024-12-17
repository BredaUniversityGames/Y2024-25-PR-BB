﻿#include "systems/physics_system.hpp"
#include "Jolt/Physics/Collision/Shape/ScaledShape.h"
#include "components/name_component.hpp"
#include "components/rigidbody_component.hpp"
#include "components/static_mesh_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "imgui.h"
#include "physics_module.hpp"
#include "renderer_module.hpp"
#include "resource_management/mesh_resource_manager.hpp"
#include <components/relationship_component.hpp>
#include <components/relationship_helpers.hpp>
#include <model_loader.hpp>
#include <renderer.hpp>

PhysicsSystem::PhysicsSystem(Engine& engine, ECSModule& ecs, PhysicsModule& physicsModule)
    : engine(engine)
    , _ecs(ecs)
    , _physicsModule(physicsModule)
{
}

void PhysicsSystem::InitializePhysicsColliders()
{
    const auto view = _ecs.GetRegistry().view<StaticMeshComponent, TransformComponent>();

    for (const auto entity : view)
    {
        StaticMeshComponent& meshComponent = view.get<StaticMeshComponent>(entity);

        // Assume worldMatrix is your 4x4 transformation matrix
        glm::mat4 worldMatrix = TransformHelpers::GetWorldMatrix(_ecs.GetRegistry(), entity);

        // Variables to store the decomposed components
        glm::vec3 scale;
        glm::quat rotationQuat;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;

        // Decompose the matrix
        glm::decompose(
            worldMatrix,
            scale,
            rotationQuat,
            translation,
            skew,
            perspective);

        // size and position
        auto& meshResourceManager = engine.GetModule<RendererModule>().GetGraphicsContext()->Resources()->MeshResourceManager();
        Vec3Range boundingBox = meshResourceManager.Access(meshComponent.mesh)->boundingBox; // * scale;
        boundingBox.min *= scale;
        boundingBox.max *= scale;

        const glm::vec3 centerPos = (boundingBox.max + boundingBox.min) * 0.5f;

        RigidbodyComponent rb(*_physicsModule.bodyInterface, entity, translation + centerPos, boundingBox, eSTATIC);

        // rotation now
        _physicsModule.bodyInterface->SetRotation(rb.bodyID, JPH::Quat(rotationQuat.x, rotationQuat.y, rotationQuat.z, rotationQuat.w), JPH::EActivation::Activate);

        _ecs.GetRegistry().emplace<RigidbodyComponent>(entity, rb);
        _ecs.GetRegistry().emplace_or_replace<UpdateMeshAndPhysics>(entity);
    }
}
entt::entity PhysicsSystem::LoadNodeRecursive(const CPUModel& models, ECSModule& ecs,
    uint32_t currentNodeIndex,
    const Hierarchy& hierarchy,
    entt::entity parent, PhysicsShapes shape)
{

    if (shape != eMESH && shape != eCONVEXHULL)
    {
        bblog::error("Shape not supported");
        return entt::null;
    }
    const entt::entity entity = ecs.GetRegistry().create();
    const Hierarchy::Node& currentNode = hierarchy.nodes[currentNodeIndex];

    ecs.GetRegistry().emplace<NameComponent>(entity).name = currentNode.name + " collider";
    ecs.GetRegistry().emplace<TransformComponent>(entity);

    ecs.GetRegistry().emplace<RelationshipComponent>(entity);
    if (parent != entt::null)
    {
        RelationshipHelpers::AttachChild(ecs.GetRegistry(), parent, entity);
    }

    TransformHelpers::SetLocalTransform(ecs.GetRegistry(), entity, currentNode.transform);

    if (currentNode.meshIndex.has_value())
    {
        auto mesh = models.meshes[currentNode.meshIndex.value().second];

        JPH::VertexList vertices;
        JPH::IndexedTriangleList triangles;

        // set verticies
        for (auto vertex : mesh.vertices)
        {
            vertices.push_back(JPH::Float3(vertex.position.x, vertex.position.y, vertex.position.z));
        }

        // set trinagles
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            JPH::IndexedTriangle tri;
            tri.mIdx[0] = mesh.indices[i + 0];
            tri.mIdx[1] = mesh.indices[i + 1];
            tri.mIdx[2] = mesh.indices[i + 2];

            triangles.push_back(tri);
        }

        const glm::vec3 position = TransformHelpers::GetWorldMatrix(_ecs.GetRegistry(), entity)[3];
        RigidbodyComponent rb;
        if (shape == eMESH)
            rb = RigidbodyComponent(*_physicsModule.bodyInterface, entt::null, position, vertices, triangles);

        if (shape == eCONVEXHULL)
            rb = RigidbodyComponent(*_physicsModule.bodyInterface, entt::null, position, vertices);

        // Assume worldMatrix is your 4x4 transformation matrix
        glm::mat4 worldMatrix = TransformHelpers::GetWorldMatrix(_ecs.GetRegistry(), entity);

        // Variables to store the decomposed components
        glm::vec3 scale;
        glm::quat rotationQuat;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;

        // Decompose the matrix
        glm::decompose(
            worldMatrix,
            scale,
            rotationQuat,
            translation,
            skew,
            perspective);

        auto result = _physicsModule.bodyInterface->GetShape(rb.bodyID)->ScaleShape(JPH::Vec3Arg(scale.x, scale.y, scale.z));
        if (result.HasError())
            bblog::error(result.GetError().c_str());

        _physicsModule.physicsSystem->GetBodyInterfaceNoLock().SetRotation(rb.bodyID, JPH::Quat(rotationQuat.x, rotationQuat.y, rotationQuat.z, rotationQuat.w), JPH::EActivation::Activate);
        _physicsModule.physicsSystem->GetBodyInterfaceNoLock().SetShape(rb.bodyID, result.Get(), true, JPH::EActivation::Activate);
        _ecs.GetRegistry()
            .emplace<RigidbodyComponent>(entity, rb);
    }

    for (const auto& nodeIndex : currentNode.children)
    {
        LoadNodeRecursive(models, _ecs, nodeIndex, hierarchy, entity, shape);
    }

    return entity;
}

void PhysicsSystem::CreateMeshCollision(const std::string& path)
{
    CPUModel models = engine.GetModule<RendererModule>().GetRenderer().get()->GetModelLoader().ExtractModelFromGltfFile(path);

    LoadNodeRecursive(models, _ecs, models.hierarchy.root, models.hierarchy, entt::null, eMESH);
}
RigidbodyComponent PhysicsSystem::CreateMeshColliderBody(const CPUMesh<Vertex>& mesh, PhysicsShapes shapeType, entt::entity entityToAttachTo)
{
    JPH::VertexList vertices;
    JPH::IndexedTriangleList triangles;

    // set verticies
    for (auto vertex : mesh.vertices)
    {
        vertices.push_back(JPH::Float3(vertex.position.x, vertex.position.y, vertex.position.z));
    }

    // set trinagles
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
    {
        JPH::IndexedTriangle tri;
        tri.mIdx[0] = mesh.indices[i + 0];
        tri.mIdx[1] = mesh.indices[i + 1];
        tri.mIdx[2] = mesh.indices[i + 2];

        triangles.push_back(tri);
    }

    RigidbodyComponent rb;
    if (shapeType == eMESH)
        rb = RigidbodyComponent(*_physicsModule.bodyInterface, entityToAttachTo, glm::vec3(0.0), vertices, triangles);
    if (shapeType == eCONVEXHULL)
        rb = RigidbodyComponent(*_physicsModule.bodyInterface, entityToAttachTo, glm::vec3(0.0), vertices);
    return rb;
}
void PhysicsSystem::CreateConvexHullCollision(const std::string& path)
{
    CPUModel models = engine.GetModule<RendererModule>().GetRenderer().get()->GetModelLoader().ExtractModelFromGltfFile(path);

    LoadNodeRecursive(models, _ecs, models.hierarchy.root, models.hierarchy, entt::null, eCONVEXHULL);
}

void PhysicsSystem::CleanUp()
{
    const auto toDestroy = _ecs.GetRegistry().view<DeleteTag, RigidbodyComponent>();
    for (const entt::entity entity : toDestroy)
    {
        const RigidbodyComponent& rb = toDestroy.get<RigidbodyComponent>(entity);
        _physicsModule.bodyInterface->RemoveBody(rb.bodyID);
        _physicsModule.bodyInterface->DestroyBody(rb.bodyID);
    }
}

void PhysicsSystem::Update(MAYBE_UNUSED ECSModule& ecs, MAYBE_UNUSED float deltaTime)
{
    // return;
    // let's check priority first between transforms and physics
    const auto transformsView = ecs.GetRegistry().view<TransformComponent, RigidbodyComponent, ToBeUpdated, StaticMeshComponent>();
    for (auto entity : transformsView)
    {
        const TransformComponent& transform = transformsView.get<TransformComponent>(entity);
        const RigidbodyComponent& rb = transformsView.get<RigidbodyComponent>(entity);
        StaticMeshComponent& meshComponent = transformsView.get<StaticMeshComponent>(entity);

        // Assume worldMatrix is your 4x4 transformation matrix
        glm::mat4 worldMatrix = TransformHelpers::GetWorldMatrix(_ecs.GetRegistry(), entity);

        // Variables to store the decomposed components
        glm::vec3 scale;
        glm::quat rotationQuat;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;

        // Decompose the matrix
        glm::decompose(
            worldMatrix,
            scale,
            rotationQuat,
            translation,
            skew,
            perspective);

        _physicsModule.bodyInterface->SetPosition(rb.bodyID, JPH::Vec3(translation.x, translation.y, translation.z), JPH::EActivation::Activate);

        auto result
            = rb.shape->ScaleShape(JPH::Vec3Arg(scale.x, scale.y, scale.z));
        if (result.HasError())
            bblog::error(result.GetError().c_str());
        _physicsModule.bodyInterface->SetRotation(rb.bodyID, JPH::Quat(rotationQuat.x, rotationQuat.y, rotationQuat.z, rotationQuat.w), JPH::EActivation::Activate);
        _physicsModule.physicsSystem->GetBodyInterfaceNoLock().SetShape(rb.bodyID, result.Get(), true, JPH::EActivation::Activate);
    }
    // TransformHelpers::ResetAllUpdateTags(ecs.GetRegistry());

    //      this part should be fast because it returns a vector of just ids not whole rigidbodies
    JPH::BodyIDVector activeBodies;
    _physicsModule.physicsSystem->GetActiveBodies(JPH::EBodyType::RigidBody, activeBodies);

    // mark all active bodies to have their mesh updated
    for (auto active_body : activeBodies)
    {
        const entt::entity entity = static_cast<entt::entity>(_physicsModule.bodyInterface->GetUserData(active_body));
        if (_ecs.GetRegistry().valid(entity))
        {
            _ecs.GetRegistry().emplace_or_replace<UpdateMeshAndPhysics>(entity);
        }
    }

    //    Update the meshes
    const auto view = _ecs.GetRegistry().view<RigidbodyComponent, StaticMeshComponent, UpdateMeshAndPhysics>();
    for (const auto entity : view)
    {
        const RigidbodyComponent& rb = view.get<RigidbodyComponent>(entity);
        const StaticMeshComponent& meshComponent = view.get<StaticMeshComponent>(entity);

        // if somehow is now not active or is static lets remove the update component
        if (!_physicsModule.bodyInterface->IsActive(rb.bodyID))
            _ecs.GetRegistry().remove<UpdateMeshAndPhysics>(entity);
        if (_physicsModule.bodyInterface->GetMotionType(rb.bodyID) == JPH::EMotionType::Static)
            _ecs.GetRegistry().remove<UpdateMeshAndPhysics>(entity);

        const auto joltMatrix = _physicsModule.bodyInterface->GetWorldTransform(rb.bodyID);

        const auto joltToGlm = ToGLMMat4(joltMatrix);

        TransformHelpers::SetWorldTransform(ecs.GetRegistry(), entity, joltToGlm);
    }
    TransformHelpers::ResetAllUpdateTags(ecs.GetRegistry());
}
void PhysicsSystem::Render(MAYBE_UNUSED const ECSModule& ecs) const
{
}
void PhysicsSystem::Inspect()
{
    ImGui::Begin("Physics System");
    const auto view = _ecs.GetRegistry().view<RigidbodyComponent>();
    static int amount = 1;
    static PhysicsShapes currentShape = eSPHERE;
    ImGui::Text("Physics Entities: %u", static_cast<unsigned int>(view.size()));
    ImGui::Text("Active bodies: %u", _physicsModule.physicsSystem->GetNumActiveBodies(JPH::EBodyType::RigidBody));

    ImGui::DragInt("Amount", &amount, 1, 1, 100);
    const char* shapeNames[] = { "Sphere", "Box", "Convex Hull" };
    const char* currentItem = shapeNames[currentShape];

    if (ImGui::BeginCombo("Select Physics Shape", currentItem)) // Dropdown name
    {
        for (int n = 0; n < IM_ARRAYSIZE(shapeNames); n++)
        {
            bool isSelected = (currentShape == n);
            if (ImGui::Selectable(shapeNames[n], isSelected))
            {
                currentShape = static_cast<PhysicsShapes>(n);
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus(); // Ensure the currently selected item is focused
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("Create Physics Entities"))
    {
        for (int i = 0; i < amount; i++)
        {
            entt::entity entity = _ecs.GetRegistry().create();
            RigidbodyComponent rb(*_physicsModule.bodyInterface, entity, currentShape);

            NameComponent node;
            node.name = "Physics Entity";
            _ecs.GetRegistry().emplace<NameComponent>(entity, node);
            _ecs.GetRegistry().emplace<RigidbodyComponent>(entity, rb);
            _physicsModule.bodyInterface->SetLinearVelocity(rb.bodyID, JPH::Vec3(0.6f, 0.0f, 0.0f));
        }
    }

    if (ImGui::Button("Create Plane Entity"))
    {
        JPH::BodyCreationSettings plane_settings(new JPH::BoxShape(JPH::Vec3(10.0f, 0.1f, 10.0f)), JPH::Vec3(0.0, 0.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, PhysicsLayers::NON_MOVING);

        entt::entity entity = _ecs.GetRegistry().create();
        RigidbodyComponent newRigidBody(*_physicsModule.bodyInterface, entity, plane_settings);
        NameComponent node;
        node.name = "Plane Entity";
        _ecs.GetRegistry().emplace<RigidbodyComponent>(entity, newRigidBody);
        _ecs.GetRegistry().emplace<NameComponent>(entity, node);
    }

    if (ImGui::Button("Create mesh collider"))
    {
        CreateMeshCollision("assets/models/monkey.gltf");
    }

    if (ImGui::Button("Create convexhull collider"))
    {
        CreateConvexHullCollision("assets/models/monkey.gltf");
    }

    if (ImGui::Button("Clear Physics Entities"))
    {
        _ecs.GetRegistry().view<RigidbodyComponent>().each([&](auto entity, auto&)
            { _ecs.DestroyEntity(entity); });
    }
    ImGui::End();
}

void PhysicsSystem::InspectRigidBody(RigidbodyComponent& rb)
{
    _physicsModule.bodyInterface->ActivateBody(rb.bodyID);
    const entt::entity entity = static_cast<entt::entity>(_physicsModule.bodyInterface->GetUserData(rb.bodyID));
    if (_ecs.GetRegistry().valid(entity))
        _ecs.GetRegistry().emplace_or_replace<UpdateMeshAndPhysics>(entity);

    ImGui::PushID(&rb.bodyID);
    JPH::Vec3 position = _physicsModule.bodyInterface->GetPosition(rb.bodyID);
    float pos[3] = { position.GetX(), position.GetY(), position.GetZ() };
    if (ImGui::DragFloat3("Position", pos, 0.1f))
    {
        _physicsModule.physicsSystem->GetBodyInterfaceNoLock().SetPosition(rb.bodyID, JPH::Vec3(pos[0], pos[1], pos[2]), JPH::EActivation::Activate);
    }

    const auto joltRotation = _physicsModule.bodyInterface->GetRotation(rb.bodyID).GetEulerAngles();
    float euler[3] = { joltRotation.GetX(), joltRotation.GetY(), joltRotation.GetZ() };
    if (ImGui::DragFloat3("Rotation", euler))
    {
        JPH::Quat newRotation = JPH::Quat::sEulerAngles(JPH::Vec3(euler[0], euler[1], euler[2]));
        _physicsModule.bodyInterface->SetRotation(rb.bodyID, newRotation, JPH::EActivation::Activate);
    }

    if (rb.shapeType == PhysicsShapes::eSPHERE)
    {
        const auto& shape = _physicsModule.bodyInterface->GetShape(rb.bodyID);
        float radius = shape->GetInnerRadius();

        if (ImGui::DragFloat("Radius", &radius, 0.1f))
        {
            radius = glm::max(radius, 0.01f);
            _physicsModule.bodyInterface->SetShape(rb.bodyID, new JPH::SphereShape(radius), true, JPH::EActivation::Activate);
        }
    }

    if (rb.shapeType == PhysicsShapes::eBOX)
    {
        const auto& shape = _physicsModule.bodyInterface->GetShape(rb.bodyID);
        auto boxShape = JPH::StaticCast<JPH::BoxShape>(shape);
        if (boxShape == nullptr)
        {
            return;
        }
        const auto joltSize = boxShape->GetHalfExtent();
        float size[3] = { joltSize.GetX(), joltSize.GetY(), joltSize.GetZ() };
        if (ImGui::DragFloat3("Size", size, 0.1f))
        {
            size[0] = glm::max(size[0], 0.1f);
            size[1] = glm::max(size[1], 0.1f);
            size[2] = glm::max(size[2], 0.1f);
            _physicsModule.bodyInterface->SetShape(rb.bodyID, new JPH::BoxShape(JPH::Vec3(size[0], size[1], size[2])), true, JPH::EActivation::Activate);
        }
    }

    JPH::EMotionType rbType = _physicsModule.bodyInterface->GetMotionType(rb.bodyID);
    const char* rbTypeNames[] = { "Static", "Kinematic", "Dynamic" };
    const char* currentItem = rbTypeNames[static_cast<uint8_t>(rbType)];

    if (ImGui::BeginCombo("Body type", currentItem))
    {
        for (int n = 0; n < IM_ARRAYSIZE(rbTypeNames); n++)
        {
            bool isSelected = (rbType == static_cast<JPH::EMotionType>(n));
            if (ImGui::Selectable(rbTypeNames[n], isSelected))
            {
                _physicsModule.bodyInterface->SetMotionType(rb.bodyID, static_cast<JPH::EMotionType>(n), JPH::EActivation::Activate);

                if (static_cast<JPH::EMotionType>(n) == JPH::EMotionType::Static)
                {
                    _physicsModule.bodyInterface->SetObjectLayer(rb.bodyID, PhysicsLayers::NON_MOVING);
                }
                else
                {
                    _physicsModule.bodyInterface->SetObjectLayer(rb.bodyID, PhysicsLayers::MOVING);
                }
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus(); // Ensure the currently selected item is focused
        }
        ImGui::EndCombo();
    }

    ImGui::PopID();
}
