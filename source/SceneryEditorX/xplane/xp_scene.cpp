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
 * xp_scene.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "xp_scene.h"
#include "xp_datarefs.h"
#include <cmath>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/scene/components/component_sets.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr const char *LOG_TAG = "XPlaneScene";
		constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;

		entt::registry *GetRegistry()
		{
			Scene *scene = Scene::GetActive();
			return scene ? &scene->GetRegistry() : nullptr;
		}

		uint64_t GetId(entt::registry &registry, const entt::entity handle)
		{
			const IDComponent *id = registry.try_get<IDComponent>(handle);
			return id ? static_cast<uint64_t>(id->pID) : 0;
		}

		entt::entity FindHandle(entt::registry &registry, const uint64_t id)
		{
			if (id == 0)
				return entt::null;

			const Entity entity = Scene::TryGetEntityWithUUID(UUID(id));
			const entt::entity handle = entity ? static_cast<entt::entity>(entity) : entt::null;
			return registry.valid(handle) ? handle : entt::null;
		}

		/* @brief Links child under parent in both RelationshipComponents. */
		void Attach(entt::registry &registry, const entt::entity child, const entt::entity parent)
		{
			const UUID parentId = registry.get<IDComponent>(parent).pID;
			const UUID childId = registry.get<IDComponent>(child).pID;
			registry.get<RelationshipComponent>(child).pParentHandle = parentId;
			registry.get<RelationshipComponent>(parent).pChildren.push_back(childId);
		}

		/* @brief Leaf name of a dataref for entity labels ("sim/graphics/animation/windsock_psi" -> "windsock_psi"). */
		std::string ShortDatarefName(const DatarefRef &ref)
		{
			const size_t slash = ref.name.rfind('/');
			std::string name = slash == std::string::npos ? ref.name : ref.name.substr(slash + 1);
			if (ref.index >= 0)
				name += "[" + std::to_string(ref.index) + "]";
			return name;
		}

		/* @brief World-space position of the primary camera (the one the renderer draws through). */
		bool GetCameraPosition(entt::registry &registry, Vec3 &position)
		{
			bool found = false;
			auto view = registry.view<TransformComponent, CameraComponent>();
			for (const entt::entity handle : view)
			{
				const CameraComponent &camera = view.get<CameraComponent>(handle);
				if (!found || camera.isPrimary)
				{
					position = view.get<TransformComponent>(handle).translation;
					found = true;
					if (camera.isPrimary)
						break;
				}
			}
			return found;
		}

		/* @brief Which LODs are drawn for a camera distance (X-Plane rules, or the component's override). */
		bool IsLodVisible(const Obj8Data &data, const int lod, const float distance, const int lodOverride)
		{
			if (lod < 0 || lodOverride == -2)
				return true;
			if (lodOverride >= 0)
				return lod == lodOverride;

			const Obj8Lod &range = data.lods[static_cast<size_t>(lod)];
			if (data.HasAdditiveLods())
				return distance < range.farDistance; // every LOD starts at 0; nearer ones add detail
			return distance >= range.nearDistance && distance < range.farDistance;
		}
	}

	// -------------------------------------------------------

	uint64_t SceneObjects::Place(const std::filesystem::path &file, const PlaceOptions &options, std::string *error)
	{
		entt::registry *registry = GetRegistry();
		if (!registry)
		{
			if (error)
				*error = "No active scene";
			return 0;
		}

		Ref<ObjectAsset> asset = ObjectAsset::Load(file, error);
		if (!asset)
			return 0;

		const Obj8Data &data = asset->GetData();
		const std::string rootName = !options.name.empty() ? options.name : file.filename().string();

		// Root: the placement transform
		const Entity root = Scene::CreateEntity(rootName);
		if (!root)
		{
			if (error)
				*error = "Could not create an entity";
			return 0;
		}

		const entt::entity rootHandle = static_cast<entt::entity>(root);
		{
			TransformComponent &transform = registry->get<TransformComponent>(rootHandle);
			transform.translation = Vec3(options.position[0], options.position[1], options.position[2]);
			// DSF headings turn clockwise seen from above: a negative turn about +Y.
			transform.SetRotationEuler(Vec3(0.0f, -options.headingDegrees * kDegToRad, 0.0f));
		}

		XPlaneObjectComponent component;
		component.asset = asset;
		component.virtualPath = options.virtualPath;

		// Animation groups (parents always precede children in Obj8Data::nodes)
		std::vector<entt::entity> nodeHandles(data.nodes.size(), entt::null);
		for (size_t i = 0; i < data.nodes.size(); ++i)
		{
			const Obj8AnimNode &node = data.nodes[i];
			std::string name = "Anim " + std::to_string(i);
			if (!node.ops.empty() && node.ops.front().dataref.IsSet())
				name += " (" + ShortDatarefName(node.ops.front().dataref) + ")";
			else if (!node.ops.empty())
				name += node.ops.front().type == Obj8AnimOp::Type::Rotate ? " (rotate)" : " (translate)";

			const Entity entity = Scene::CreateEntity(name);
			nodeHandles[i] = static_cast<entt::entity>(entity);
			const entt::entity parent = node.parent >= 0 ? nodeHandles[static_cast<size_t>(node.parent)] : rootHandle;
			Attach(*registry, nodeHandles[i], parent);
			component.nodeEntities.push_back(GetId(*registry, nodeHandles[i]));
		}

		// Batches: one Renderable each
		const std::vector<ObjectBatch> &batches = asset->GetBatches();
		for (size_t i = 0; i < batches.size(); ++i)
		{
			const ObjectBatch &batch = batches[i];
			std::string name = "Mesh " + std::to_string(i);
			if (batch.lod >= 0)
			{
				const Obj8Lod &lod = data.lods[static_cast<size_t>(batch.lod)];
				name += " [LOD " + std::to_string(static_cast<int>(lod.nearDistance)) + "-" + std::to_string(static_cast<int>(lod.farDistance)) + "m]";
			}
			if (batch.draped)
				name += " (draped)";

			const Entity entity = Scene::CreateEntity(name);
			const entt::entity handle = static_cast<entt::entity>(entity);
			const entt::entity parent = batch.node >= 0 ? nodeHandles[static_cast<size_t>(batch.node)] : rootHandle;
			Attach(*registry, handle, parent);
			component.batchEntities.push_back(GetId(*registry, handle));

			if (Entity *stored = Scene::GetStoredEntity(entity))
			{
				if (Renderable *renderable = stored->AddComponent<Renderable>())
				{
					renderable->SetMesh(asset->GetMesh(), batch.subMesh);
					renderable->SetMaterial(Ref<MaterialAsset>(asset->GetMaterial(batch.material)));
					renderable->SetFlag(CastsShadows, batch.castShadow);
				}
			}
		}

		// Expose the object's datarefs as editor controls.
		for (const Obj8DatarefUsage &usage : data.datarefs)
			Datarefs::Register(usage);

		registry->emplace_or_replace<XPlaneObjectComponent>(rootHandle, std::move(component));
		const uint64_t rootId = GetId(*registry, rootHandle);

		SEDX_CORE_INFO_TAG(LOG_TAG, "Placed '{}' ({} batches, {} animation groups){}", rootName, batches.size(), data.nodes.size(),
			options.virtualPath.empty() ? std::string() : " as " + options.virtualPath);

		Tick(0.0);
		if (options.frameCamera)
			FrameObject(rootId);
		return rootId;
	}

	void SceneObjects::Tick(const double deltaSeconds)
	{
		Datarefs::Tick(deltaSeconds);
		ObjectAsset::SetLitTexturesVisible(Datarefs::GetNightFactor() > 0.5f);

		entt::registry *registry = GetRegistry();
		if (!registry)
			return;

		Vec3 cameraPosition(0.0f, 0.0f, 0.0f);
		const bool hasCamera = GetCameraPosition(*registry, cameraPosition);
		const DatarefReader reader = Datarefs::GetReader();

		auto view = registry->view<XPlaneObjectComponent>();
		for (const entt::entity rootHandle : view)
		{
			XPlaneObjectComponent &object = view.get<XPlaneObjectComponent>(rootHandle);
			if (!object.asset)
				continue;

			const Obj8Data &data = object.asset->GetData();

			// Animation groups
			if (object.animate)
			{
				for (size_t i = 0; i < object.nodeEntities.size() && i < data.nodes.size(); ++i)
				{
					const entt::entity handle = FindHandle(*registry, object.nodeEntities[i]);
					if (handle == entt::null)
						continue;

					const RigidTransform local = EvaluateAnimNode(data.nodes[i], reader);
					TransformComponent &transform = registry->get<TransformComponent>(handle);
					transform.translation = Vec3(local.translation[0], local.translation[1], local.translation[2]);
					transform.SetRotation(Quat(local.rotation[3], local.rotation[0], local.rotation[1], local.rotation[2]));
				}
			}

			// Camera distance for LOD selection, from the root's world position
			float distance = 0.0f;
			if (hasCamera)
			{
				if (Entity *stored = Scene::GetStoredEntity(Entity(rootHandle, Scene::GetActive())))
				{
					const xMath::Matrix world = stored->GetMatrix();
					const Vec3 delta(world.m30 - cameraPosition.x, world.m31 - cameraPosition.y, world.m32 - cameraPosition.z);
					distance = std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
				}
			}
			object.cameraDistance = distance;

			// Batches: LOD + hide / show
			object.visibleBatches = 0;
			object.visibleTriangles = 0;
			const std::vector<ObjectBatch> &batches = object.asset->GetBatches();
			for (size_t i = 0; i < object.batchEntities.size() && i < batches.size(); ++i)
			{
				const entt::entity handle = FindHandle(*registry, object.batchEntities[i]);
				Entity *stored = handle != entt::null ? Scene::GetStoredEntity(Entity(handle, Scene::GetActive())) : nullptr;
				if (!stored)
					continue;

				const ObjectBatch &batch = batches[i];
				const bool visible = object.visible && IsLodVisible(data, batch.lod, distance, object.lodOverride) &&
									 EvaluateVisibility(data, batch.visibility, reader);
				stored->SetActive(visible);
				if (visible)
				{
					++object.visibleBatches;
					object.visibleTriangles += batch.triangleCount;
				}
			}
		}
	}

	XPlaneObjectComponent *SceneObjects::FindObject(const uint64_t entityId, uint64_t *rootId)
	{
		entt::registry *registry = GetRegistry();
		if (!registry)
			return nullptr;

		entt::entity handle = FindHandle(*registry, entityId);
		for (int depth = 0; handle != entt::null && depth < 64; ++depth)
		{
			if (XPlaneObjectComponent *object = registry->try_get<XPlaneObjectComponent>(handle))
			{
				if (rootId)
					*rootId = GetId(*registry, handle);
				return object;
			}

			const RelationshipComponent *relationship = registry->try_get<RelationshipComponent>(handle);
			handle = relationship ? FindHandle(*registry, static_cast<uint64_t>(relationship->pParentHandle)) : entt::null;
		}
		return nullptr;
	}

	void SceneObjects::FrameObject(const uint64_t rootId)
	{
		entt::registry *registry = GetRegistry();
		XPlaneObjectComponent *object = registry ? FindObject(rootId) : nullptr;
		const entt::entity rootHandle = registry ? FindHandle(*registry, rootId) : entt::null;
		if (!object || !object->asset || rootHandle == entt::null)
			return;

		// Bounds center in world space (placement translation + heading only; the object is not scaled)
		const float *bmin = object->asset->GetBoundsMin();
		const float *bmax = object->asset->GetBoundsMax();
		const float local[3] = { (bmin[0] + bmax[0]) * 0.5f, (bmin[1] + bmax[1]) * 0.5f, (bmin[2] + bmax[2]) * 0.5f };
		const float extent[3] = { bmax[0] - bmin[0], bmax[1] - bmin[1], bmax[2] - bmin[2] };
		const float radius = std::max(0.5f, 0.5f * std::sqrt(extent[0] * extent[0] + extent[1] * extent[1] + extent[2] * extent[2]));

		Vec3 center(local[0], local[1], local[2]);
		if (Entity *stored = Scene::GetStoredEntity(Entity(rootHandle, Scene::GetActive())))
		{
			const xMath::Matrix world = stored->GetMatrix();
			center = Vec3(local[0] * world.m00 + local[1] * world.m10 + local[2] * world.m20 + world.m30,
						  local[0] * world.m01 + local[1] * world.m11 + local[2] * world.m21 + world.m31,
						  local[0] * world.m02 + local[1] * world.m12 + local[2] * world.m22 + world.m32);
		}

		// Three-quarter view from the south-east, far enough that the bounding sphere fits a 60 degree view.
		auto view = registry->view<TransformComponent, CameraComponent>();
		for (const entt::entity handle : view)
		{
			if (!view.get<CameraComponent>(handle).isPrimary)
				continue;

			constexpr float yaw = -35.0f * kDegToRad;
			constexpr float pitch = -25.0f * kDegToRad;
			const Vec3 forward(std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw));
			const float distance = radius / std::sin(30.0f * kDegToRad) * 1.15f;

			TransformComponent &transform = view.get<TransformComponent>(handle);
			transform.translation = center - forward * distance;
			transform.SetRotationEuler(Vec3(pitch, yaw, 0.0f));
			break;
		}
	}

	void SceneObjects::Shutdown()
	{
		if (entt::registry *registry = GetRegistry())
		{
			std::vector<uint64_t> roots;
			for (const entt::entity handle : registry->view<XPlaneObjectComponent>())
				roots.push_back(GetId(*registry, handle));

			for (const uint64_t root : roots)
			{
				if (const Entity entity = Scene::TryGetEntityWithUUID(UUID(root)))
					Scene::DestroyEntity(entity);
			}
		}

		ObjectAsset::ClearCache();
		TextureLoader::ClearCache();
	}

	size_t SceneObjects::GetObjectCount()
	{
		entt::registry *registry = GetRegistry();
		return registry ? registry->view<XPlaneObjectComponent>().size() : 0;
	}

}

// -------------------------------------------------------
