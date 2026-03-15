/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * component_sets.h
 * -------------------------------------------------------
 * Created: 3/8/2025
 * -------------------------------------------------------
 */
// ReSharper disable CppNonExplicitConvertingConstructor
#pragma once
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/core/base.h>
#include <SceneryEditorX/core/identifiers/uuid.h>
#include <xMath/includes/xmath.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{

/**
	 * @struct IDComponent
	 * @brief Component that stores a unique identifier (UUID) for an entity.
	 */
	struct IDComponent
	{
		UUID pID = UUID();
	};

	// -------------------------------------------------------

	/**
	 * @struct TagComponent
	 * @brief Component that stores a human-readable name or tag for an entity.
	 */
	struct TagComponent
	{

		std::string pTag;

		TagComponent() = default;
		TagComponent(const TagComponent& other) = default;
		TagComponent(std::string tag) : pTag(std::move(tag)) {}

		operator std::string& () { return pTag; }
		operator const std::string& () const { return pTag; }
	};

	// -------------------------------------------------------

	/**
	 * @struct RelationshipComponent
	 * @brief Component that stores parent-child relationships between entities.
	 */
	struct RelationshipComponent
	{
		UUID pParentHandle{0};
		std::vector<UUID> pChildren;

		RelationshipComponent() = default;
		RelationshipComponent(const RelationshipComponent &other) = default;
		RelationshipComponent(UUID parent) : pParentHandle(std::move(parent)) {}
	};

	// -------------------------------------------------------

	/**
	 * @struct PrefabComponent
	 * @brief Component that stores prefab information for an entity.
	 */
	struct PrefabComponent
	{
		UUID prefabId;
		UUID entityId;
	};

	// -------------------------------------------------------

	/**
	 * @struct TransformComponent
	 * @brief Component that stores transformation data (translation, rotation, scale) for an entity.
	 */
	struct TransformComponent
	{

		xMath::Vec3 translation		= {0.0f, 0.0f, 0.0f};
		xMath::Vec3 rotationEuler	= {0.0f, 0.0f, 0.0f};			// Editor-friendly
		xMath::Quat rotation		= {1.0f, 0.0f, 0.0f, 0.0f};	// Math-friendly
		xMath::Vec3 scale			= {1.0f, 1.0f, 1.0f};

		TransformComponent() = default;

		/**
		 * @brief Constructs a TransformComponent by copying another TransformComponent.
		 * @param other The TransformComponent to copy.
		 */
		TransformComponent(const TransformComponent &other) = default;

		/**
		 * @brief Constructs a TransformComponent with a given translation.
		 * @param translation The translation vector to initialize the component with.
		 */
		TransformComponent(const Vec3 &translation) : translation(translation) {}

		/**
		 * @brief Gets the local transformation matrix of the component.
		 * @return The local transformation matrix representing the component's transformation.
		 */
		[[nodiscard]] xMath::Mat4 GetLocalTransform() const
		{
			return xMath::Mat4::Translate(translation) * rotation.ToMatrix() * xMath::Mat4::Scale(scale);
		}

		/**
		 * @brief Gets the transformation matrix of the component.
		 * @return The transformation matrix representing the component's transformation.
		 */
		[[nodiscard]] Mat4 GetTransform() const
		{
			// M = T * R * S
			return Mat4::Translate(translation) * m_Rotation.ToMatrix() * Mat4::Scale(scale);
		}

		/**
		 * @brief Sets the transformation of the component.
		 * @param transform The transformation matrix to set.
		 */
		void SetTransform(const Mat4 &transform)
		{
			Transforms::Decompose(transform, translation, m_Rotation, scale);
			m_RotationEuler = m_Rotation.ToEulerRadians(); // Store editor euler in radians
		}

		/**
		 * @brief Gets the rotation of the component as Euler angles.
		 * @return The Euler angles representing the rotation in radians.
		 */
		[[nodiscard]] Vec3 GetRotationEuler() const { return m_RotationEuler; }

		/**
		 * @brief Sets the rotation of the component using Euler angles.
		 * @param euler The Euler angles representing the rotation in radians.
		 */
		void SetRotationEuler(const Vec3 &euler)
		{
			m_RotationEuler = euler; // euler is in radians
			m_Rotation = Quat::EulerRadians(m_RotationEuler);
		}

		/**
		 * @brief Gets the rotation of the component as a quaternion.
		 * @return The quaternion representing the rotation.
		 */
		[[nodiscard]] Quat GetRotation() const { return m_Rotation; }

		/**
		 * @brief Sets the rotation of the component using a quaternion.
		 * @param quat The quaternion representing the rotation.
		 */
		void SetRotation(const Quat &quat)
		{
			// wrap given euler angles to range [-pi, pi]
			auto wrap_to_pi = [](Vec3 v)
			{
				auto wrap = [](float a)
				{
					// Wrap to [-PI, PI]
					a = std::fmod(a + PI, TWO_PI);
					if (a < 0.0f) a += TWO_PI;
					return a - PI;
				};
				return Vec3(wrap(v.x), wrap(v.y), wrap(v.z));
			};

			auto originalEuler = m_RotationEuler;
			m_Rotation = quat;
			m_RotationEuler = m_Rotation.ToEulerRadians();

			/**
			 * A given quaternion can be represented by many Euler angle triplets (technically infinitely many),
			 * and our ToEulerRadians() returns one canonical solution which may not match the previous user-facing angles.
			 * Evaluate a small set of equivalent alternatives and pick the one closest to the original to avoid visual 180° flips.
			 */
			Vec3 alternate1 = {m_RotationEuler.x - PI,
							   PI - m_RotationEuler.y,
							   m_RotationEuler.z - PI};
			Vec3 alternate2 = {m_RotationEuler.x + PI,
							   PI - m_RotationEuler.y,
							   m_RotationEuler.z - PI};
			Vec3 alternate3 = {m_RotationEuler.x + PI,
							   PI - m_RotationEuler.y,
							   m_RotationEuler.z + PI};
			Vec3 alternate4 = {m_RotationEuler.x - PI,
							   PI - m_RotationEuler.y,
							   m_RotationEuler.z + PI};

			// We pick the alternative that is closest to the original value.
			float distance0 = Length2(wrap_to_pi(m_RotationEuler - originalEuler));
			float distance1 = Length2(wrap_to_pi(alternate1 - originalEuler));
			float distance2 = Length2(wrap_to_pi(alternate2 - originalEuler));
			float distance3 = Length2(wrap_to_pi(alternate3 - originalEuler));
			float distance4 = Length2(wrap_to_pi(alternate4 - originalEuler));

			float best = distance0;
			if (distance1 < best)
			{
				best = distance1;
				m_RotationEuler = alternate1;
			}

			if (distance2 < best)
			{
				best = distance2;
				m_RotationEuler = alternate2;
			}

			if (distance3 < best)
			{
				best = distance3;
				m_RotationEuler = alternate3;
			}

			if (distance4 < best)
			{
				best = distance4;
				m_RotationEuler = alternate4;
			}

			m_RotationEuler = wrap_to_pi(m_RotationEuler);
		}

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
		 * and quats for everything else. The two are maintained in-sync via the SetRotation()
		 * methods.
		 */
		Vec3 m_RotationEuler = {0.0f, 0.0f, 0.0f};
		Quat m_Rotation = {1.0f, 0.0f, 0.0f, 0.0f};

		//friend class SceneSerializer;
	};

	// -------------------------------------------------------

	/**
	 * @struct MeshComponent
	 * @brief Component that represents a mesh attached to an entity.  
	 * The mesh can be static or dynamic (skinned/rigged).  
	 * If the mesh is dynamic, then the entity with this component is considered the "root" of the dynamic mesh hierarchy, 
	 * and all its descendants with MeshTagComponents are considered part of the same dynamic mesh.
	 */
	struct MeshComponent
	{
		AssetHandle mesh;
	};

	/**
	 * @struct MeshTagComponent
	 * @brief Component that tags an entity as part of a dynamic mesh hierarchy.
	 */
	struct MeshTagComponent
	{
		UUID meshEntity;
	};

	/*
	// The actual (sub)meshes of a dynamic mesh
	struct SubmeshComponent
	{
		AssetHandle mesh;
		Ref<SceneryEditorX::MaterialTable> materialTable = CreateRef<SceneryEditorX::MaterialTable>();
		std::vector<UUID> boneEntityIds; // TODO: BoneEntityIds should be a separate component (not all meshes need this).  If mesh is rigged, these are the entities whose transforms will be used to "skin" the rig.
		uint32_t submeshIndex = 0;
		bool visible = true;

		SubmeshComponent() = default;
		SubmeshComponent(const SubmeshComponent &other) : mesh(other.mesh), materialTable(CreateRef<SceneryEditorX::MaterialTable>(other.materialTable)),
		boneEntityIds(other.boneEntityIds), submeshIndex(other.submeshIndex), visible(other.visible) {}

		SubmeshComponent(const AssetHandle &mesh, uint32_t submeshIndex = 0) : mesh(mesh), submeshIndex(submeshIndex) {}
	};
	*/

	// -------------------------------------------------------

	/*
	struct StaticMeshComponent
	{
		AssetHandle staticMesh;
		Ref<SceneryEditorX::MaterialTable> materialTable = CreateRef<SceneryEditorX::MaterialTable>();
		bool visible = true;

		StaticMeshComponent() = default;
		StaticMeshComponent(const StaticMeshComponent &other) : staticMesh(other.staticMesh), materialTable(CreateRef<SceneryEditorX::MaterialTable>(other.materialTable)), visible(other.visible){}
		StaticMeshComponent(const AssetHandle &staticMesh) : staticMesh(staticMesh) {}
	};
	*/

	// -------------------------------------------------------

	/*
	struct AnimationComponent
	{
		AssetHandle animationGraphHandle;
		std::vector<UUID> boneEntityIds; /// AnimationGraph refers to a skeleton.  Skeleton has a collection of bones.  Each bone affects the transform of an entity. These are those entities.
		Mat3 rootBoneTransform; /// Transform of the animated root bone relative to the entity that this AnimationComponent belongs to.  This is used to rotate/scale root motion before applying it to the entity.
		Ref<AnimationGraph::AnimationGraph> animationGraph;

		// Note: generally if you copy an AnimationComponent, then you will need to:
		// A) Reset the bone entity ids (e.g.to point to copied entities that the copied component belongs to).  See Scene::DuplicateEntity()
		// B) Create a new independent AnimationGraph instance.  See Scene::DuplicateEntity()
	};
	*/

	// -------------------------------------------------------

	/*
	struct SpriteRendererComponent
	{
		Vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
		AssetHandle texture;
		float tilingFactor = 1.0f;
		Vec2 uvStart{0.0f, 0.0f};
		Vec2 uvEnd{1.0f, 1.0f};
		bool screenSpace = false;

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent &other) = default;
	};
	*/

	// -------------------------------------------------------

	/*
	struct TextComponent
	{
		std::string textString;
		size_t textHash = 0;

		// Font
		AssetHandle fontHandle;
		Vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
		float lineSpacing = 0.0f;
		float kerning = 0.0f;

		// Layout
		float maxWidth = 10.0f;

		bool screenSpace = false;
		bool dropShadow = false;
		float shadowDistance = 0.0f;
		Vec4 shadowColor = {0.0f, 0.0f, 0.0f, 1.0f};

		TextComponent() = default;
		TextComponent(const TextComponent &other) = default;
	};
	*/

	// -------------------------------------------------------

	/**
	 * @struct DirectionalLightComponent
	 * @brief Component that represents a directional light source in the scene, such as the sun.
	 */
	struct DirectionalLightComponent
	{
		Vec3 radiance		= {1.0f, 1.0f, 1.0f};
		float intensity		= 1.0f;
		float lightSize		= 0.5f; // For PCSS
		float shadowAmount	= 1.0f;

		bool castShadows	= true;
		bool softShadows	= true;
	};

	// -------------------------------------------------------

	/**
	 * @struct PointLightComponent
	 * @brief Component that represents a point light source in the scene, which emits light in all directions from a single point.
	 */
	struct PointLightComponent
	{
		Vec3 radiance		= {1.0f, 1.0f, 1.0f};
		float radius		= 10.f;
		float falloff		= 1.f;
		float minRadius		= 1.f;
		float intensity		= 1.0f;
		float lightSize		= 0.5f; // For PCSS

		bool castsShadows	= true;
		bool softShadows	= true;
	};

	// -------------------------------------------------------

	/**
	 * @struct SpotLightComponent
	 * @brief Component that represents a spotlight source in the scene, which emits light in a cone shape from a single point.
	 */
	struct SpotLightComponent
	{
		Vec3 radiance			= {1.0f, 1.0f, 1.0f};
		float range				= 10.0f;
		float angle				= 60.0f;
		float falloff			= 1.0f;
		float intensity			= 1.0f;
		float angleAttenuation	= 5.0f;

		bool softShadows		= false;
		bool castsShadows		= false;
	};

	// -------------------------------------------------------

	/**
	 * @struct CameraComponent
	 * @brief Component that represents a camera in the scene.
	 */
	struct CameraComponent
	{
		float horizontalFov_Rad = 90.0f * xMath::DEG_TO_RAD;
		float nearPlane			= 0.1f;
		float farPlane			= 10000.0f; // Reverse-Z friendly
		
		enum class ProjectionType { Perspective = 0, Orthographic = 1 };
		ProjectionType projectionType = ProjectionType::Perspective;
	
		bool isPrimary = true; // Used by the renderer to find the main view
		bool useJitter = true; // For TAA/DLSS in your Vulkan pipeline
	};

	// -------------------------------------------------------

	/*
	struct SkyLightComponent
	{
		AssetHandle sceneEnvironment;
		float intensity = 1.0f;
		float lod = 0.0f;
		bool dynamicSky = false;
		Vec3 turbidityAzimuthInclination = {2.0, 0.0, 0.0};
	};
	*/

	// -------------------------------------------------------

	/*
	struct TileRendererComponent
	{
		AssetHandle staticMesh;
		uint32_t width  = 128;
		uint32_t height = 128;

		std::vector<Ref<MaterialTable>> materials{1};	// Width * Height material IDs
		std::vector<uint8_t> materialIDs{1};			// Width * Height material IDs

		TileRendererComponent() { materials[0] = CreateRef<MaterialTable>(); }
	};
	*/

}

// -------------------------------------------------------
