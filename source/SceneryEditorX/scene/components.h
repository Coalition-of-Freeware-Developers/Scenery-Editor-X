/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* components.h
* -------------------------------------------------------
* Created: 3/8/2025
* -------------------------------------------------------
*/
#pragma once

#include <SceneryEditorX/core/identifiers/uuid.h>
#include <SceneryEditorX/scene/material.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <SceneryEditorX/utils/math/transforms.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
	struct IDComponent
	{
        UUID ID = UUID(0);
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent& other) = default;
		TagComponent(std::string tag) : Tag(std::move(tag)) {}

		operator std::string& () { return Tag; }
		operator const std::string& () const { return Tag; }
	};

    struct RelationshipComponent
    {
        UUID ParentHandle = UUID(0);
        std::vector<UUID> Children;

        RelationshipComponent() = default;
        RelationshipComponent(const RelationshipComponent &other) = default;
        RelationshipComponent(const UUID &parent) : ParentHandle(parent) { }
    };

    struct PrefabComponent
    {
        UUID PrefabID = UUID(0);
        UUID EntityID = UUID(0);
    };

    struct TransformComponent
    {
        Vec3 Translation = {0.0f, 0.0f, 0.0f};
        Vec3 Scale = {1.0f, 1.0f, 1.0f};

    private:
		/**
         * These are private so that you are forced to set them via
         * SetRotation() or SetRotationEuler()
         * This avoids situation where one of them gets set and the other is forgotten.
         *
         * Why do we need both a quat and Euler angle representation for rotation?
         * Because Euler suffers from gimbal lock -> rotations should be stored as quaternions.
         *
         * BUT: quaternions are confusing, and humans like to work with Euler angles.
         * We cannot store just the quaternions and translate to/from Euler because the conversion
         * Euler -> quat -> Euler is not invariant.
         *
         * It's also sometimes useful to be able to store rotations > 360 degrees which
         * quats do not support.
         *
         * Accordingly, we store Euler for "editor" stuff that humans work with,
         * and quats for everything else.  The two are maintained in-sync via the SetRotation()
         * methods.
		 */
        Vec3 RotationEuler = {0.0f, 0.0f, 0.0f};
        glm::quat Rotation = {1.0f, 0.0f, 0.0f, 0.0f};

    public:
        TransformComponent() = default;
        TransformComponent(const TransformComponent &other) = default;
        TransformComponent(const Vec3 &translation) : Translation(translation) {}

        [[nodiscard]] Mat4 GetTransform() const
        {
            return glm::translate(Mat4(1.0f), Translation) * glm::toMat4(Rotation) *
                   glm::scale(Mat4(1.0f), Scale);
        }

        void SetTransform(const Mat4 &transform)
        {
            Utils::DecomposeTransform(transform, Translation, Rotation, Scale);
            RotationEuler = glm::eulerAngles(Rotation);
        }

        [[nodiscard]] Vec3 GetRotationEuler() const { return RotationEuler; }

        void SetRotationEuler(const Vec3 &euler)
        {
            RotationEuler = euler;
            Rotation = glm::quat(RotationEuler);
        }

        [[nodiscard]] glm::quat GetRotation() const { return Rotation; }

        void SetRotation(const glm::quat &quat)
        {
            /// wrap given euler angles to range [-pi, pi]
            auto wrapToPi = [](Vec3 v)
            {
                return glm::mod(v + glm::pi<float>(), 2.0f * glm::pi<float>()) - glm::pi<float>();
            };

            auto originalEuler = RotationEuler;
            Rotation = quat;
            RotationEuler = glm::eulerAngles(Rotation);

            // A given quat can be represented by many Euler angles (technically infinitely many),
            // and glm::eulerAngles() can only give us one of them which may or may not be the one we want.
            // Here we have a look at some likely alternatives and pick the one that is closest to the original Euler angles.
            // This is an attempt to avoid sudden 180deg flips in the Euler angles when we SetRotation(quat).

            Vec3 alternate1 = {RotationEuler.x - glm::pi<float>(),
                                    glm::pi<float>() - RotationEuler.y,
                                    RotationEuler.z - glm::pi<float>()};
            Vec3 alternate2 = {RotationEuler.x + glm::pi<float>(),
                                    glm::pi<float>() - RotationEuler.y,
                                    RotationEuler.z - glm::pi<float>()};
            Vec3 alternate3 = {RotationEuler.x + glm::pi<float>(),
                                    glm::pi<float>() - RotationEuler.y,
                                    RotationEuler.z + glm::pi<float>()};
            Vec3 alternate4 = {RotationEuler.x - glm::pi<float>(),
                                    glm::pi<float>() - RotationEuler.y,
                                    RotationEuler.z + glm::pi<float>()};

            /// We pick the alternative that is closest to the original value.
            float distance0 = glm::length2(wrapToPi(RotationEuler - originalEuler));
            float distance1 = glm::length2(wrapToPi(alternate1 - originalEuler));
            float distance2 = glm::length2(wrapToPi(alternate2 - originalEuler));
            float distance3 = glm::length2(wrapToPi(alternate3 - originalEuler));
            float distance4 = glm::length2(wrapToPi(alternate4 - originalEuler));

            float best = distance0;
            if (distance1 < best)
            {
                best = distance1;
                RotationEuler = alternate1;
            }
            if (distance2 < best)
            {
                best = distance2;
                RotationEuler = alternate2;
            }
            if (distance3 < best)
            {
                best = distance3;
                RotationEuler = alternate3;
            }
            if (distance4 < best)
            {
                best = distance4;
                RotationEuler = alternate4;
            }

            RotationEuler = wrapToPi(RotationEuler);
        }

        //friend class SceneSerializer;
    };

    /// Entity with this component is the "root" of a dynamic mesh
    struct MeshComponent
    {
        AssetHandle Mesh;
    };

    /// Tags entities that are part of a dynamic mesh hierarchy
    struct MeshTagComponent
    {
        UUID MeshEntity;
    };

    /// The actual (sub)meshes of a dynamic mesh
    struct SubmeshComponent
    {
        AssetHandle Mesh;
        // ReSharper disable once CppRedundantQualifier
        Ref<SceneryEditorX::MaterialTable> MaterialTable = CreateRef<SceneryEditorX::MaterialTable>();
        std::vector<UUID> BoneEntityIds; /// TODO: BoneEntityIds should be a separate component (not all meshes need this).  If mesh is rigged, these are the entities whose transforms will be used to "skin" the rig.
        uint32_t SubmeshIndex = 0;
        bool Visible = true;

        SubmeshComponent() = default;
        SubmeshComponent(const SubmeshComponent &other) : Mesh(other.Mesh),
			MaterialTable(CreateRef<SceneryEditorX::MaterialTable>(other.MaterialTable)), BoneEntityIds(other.BoneEntityIds), SubmeshIndex(other.SubmeshIndex), Visible(other.Visible)
        {
        }

        SubmeshComponent(const AssetHandle &mesh, uint32_t submeshIndex = 0) :
            Mesh(mesh), SubmeshIndex(submeshIndex)
        {
        }
    };

    struct StaticMeshComponent
    {
        AssetHandle StaticMesh;
        // ReSharper disable once CppRedundantQualifier
        Ref<SceneryEditorX::MaterialTable> MaterialTable = CreateRef<SceneryEditorX::MaterialTable>();
        bool Visible = true;

        StaticMeshComponent() = default;
        StaticMeshComponent(const StaticMeshComponent &other) : StaticMesh(other.StaticMesh), MaterialTable(CreateRef<SceneryEditorX::MaterialTable>(other.MaterialTable)), Visible(other.Visible)
        {
        }

        StaticMeshComponent(const AssetHandle &staticMesh) : StaticMesh(staticMesh)
        {
        }
    };

    /*
    struct AnimationComponent
    {
        AssetHandle AnimationGraphHandle;
        std::vector<UUID> BoneEntityIds; /// AnimationGraph refers to a skeleton.  Skeleton has a collection of bones.  Each bone affects the transform of an entity. These are those entities.
        Mat3 RootBoneTransform; /// Transform of the animated root bone relative to the entity that this AnimationComponent belongs to.  This is used to rotate/scale root motion before applying it to the entity.
        Ref<AnimationGraph::AnimationGraph> AnimationGraph;

        /// Note: generally if you copy an AnimationComponent, then you will need to:
        /// A) Reset the bone entity ids (e.g.to point to copied entities that the copied component belongs to).  See Scene::DuplicateEntity()
        /// B) Create a new independent AnimationGraph instance.  See Scene::DuplicateEntity()
    };
    */

    struct SpriteRendererComponent
    {
        Vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};
        AssetHandle Texture = UUID(0);
        float TilingFactor = 1.0f;
        Vec2 UVStart{0.0f, 0.0f};
        Vec2 UVEnd{1.0f, 1.0f};
        bool ScreenSpace = false;

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent &other) = default;
    };

    struct TextComponent
    {
        std::string TextString;
        size_t TextHash = 0;

        /// Font
        AssetHandle FontHandle;
        Vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};
        float LineSpacing = 0.0f;
        float Kerning = 0.0f;

        /// Layout
        float MaxWidth = 10.0f;

        bool ScreenSpace = false;
        bool DropShadow = false;
        float ShadowDistance = 0.0f;
        Vec4 ShadowColor = {0.0f, 0.0f, 0.0f, 1.0f};

        TextComponent() = default;
        TextComponent(const TextComponent &other) = default;
    };

    enum class LightType : uint8_t
    {
        None = 0,
        Directional = 1,
        Point = 2,
        Spot = 3
    };

    struct DirectionalLightComponent
    {
        Vec3 Radiance = {1.0f, 1.0f, 1.0f};
        float Intensity = 1.0f;
        float LightSize = 0.5f; /// For PCSS
        float ShadowAmount = 1.0f;

        bool CastShadows = true;
        bool SoftShadows = true;
    };

    struct PointLightComponent
    {
        Vec3 Radiance = {1.0f, 1.0f, 1.0f};
        float Radius = 10.f;
        float Falloff = 1.f;
        float MinRadius = 1.f;
        float Intensity = 1.0f;
        float LightSize = 0.5f; /// For PCSS

        bool CastsShadows = true;
        bool SoftShadows = true;
    };

    struct SpotLightComponent
    {
        Vec3 Radiance{1.0f};
        float Range = 10.0f;
        float Angle = 60.0f;
        float Falloff = 1.0f;
        float Intensity = 1.0f;
        float AngleAttenuation = 5.0f;

        bool SoftShadows = false;
        bool CastsShadows = false;
    };

    struct SkyLightComponent
    {
        AssetHandle SceneEnvironment;
        float Intensity = 1.0f;
        float Lod = 0.0f;
        bool DynamicSky = false;
        Vec3 TurbidityAzimuthInclination = {2.0, 0.0, 0.0};
    };

    struct TileRendererComponent
    {
        AssetHandle StaticMesh;
        uint32_t Width = 128;
        uint32_t Height = 128;

        // ReSharper disable once CppRedundantQualifier
        std::vector<Ref<SceneryEditorX::MaterialTable>> Materials{1};	/// Width * Height material IDs
        std::vector<uint8_t> MaterialIDs{1};								/// Width * Height material IDs

        // ReSharper disable once CppRedundantQualifier
        TileRendererComponent() { Materials[0] = CreateRef<SceneryEditorX::MaterialTable>(); }
    };

}

/// -------------------------------------------------------
