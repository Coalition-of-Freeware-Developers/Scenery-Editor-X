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
 * scene.cpp
 * -------------------------------------------------------
 * Created: 11/4/2025
 * -------------------------------------------------------
 */
#include "scene.h"
#include "entity.h"
#include "material.h"
#include "components/renderable.h"
#include "SceneryEditorX/core/time/date_time.h"
#include "SceneryEditorX/renderer/vulkan/image_resource.h"
#include "SceneryEditorX/renderer/renderer.h"
#include "components/component_sets.h"
#include "components/lights.h"
#include "components/wind.h"
#include <algorithm>
#include <unordered_set>
#include <filesystem>
#include <Editor/modules/scene_render.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/scene/camera.h>
#include <entt/entity/fwd.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{

	Ref<Camera> Scene::m_Camera = nullptr;
	static Scope<Scene> s_ActiveScene = nullptr;
	static std::unordered_map<entt::entity, Scope<Entity>> s_EntityStorage;
	static std::vector<Entity*> s_EntityPointers;
	static std::vector<Entity*> s_LightEntities;       // entities subset that contains only lights (cached, see RefreshEntityLists)
	static std::vector<Entity*> s_RenderableEntities;  // entities subset that contains only renderables (cached)
	static std::vector<MaterialAsset*> s_RenderableMaterials; // distinct materials of s_RenderableEntities (cached)
	static uint64_t s_StructureVersion = 1;            // see Scene::GetStructureVersion()
	static uint64_t s_EntityListsVersion = 0;          // structure version the cached lists were built at
	static uint64_t s_DestroyVersion = 1;              // see Scene::GetDestroyVersion()
	static std::vector<const Entity*> s_DestroyedEntities; // destroyed, still listed in s_EntityPointers (CompactEntityPointers)
	static BoundingBox s_BoundingBox = BoundingBox::UNIT;
	static std::string s_FilePath;
	static std::string s_SceneName; // cached to avoid per-frame allocation
	static std::string s_SceneDescription;
	static std::mutex s_EntityAccess_Mutex;
	static std::atomic<bool> s_Resolve = false;
	static std::unordered_map<uint64_t, uint32_t> s_EntityStates; // stores: low 8 bits for flags, next 8 for component count, next 8 for cull mode, next 8 for light type
	// Change tracking, one hash per entry of s_RenderableMaterials / s_LightEntities (rebuilt with those lists, which
	// counts as a change): things that change the nature of a material / light for rendering
	static std::vector<size_t> s_MaterialStateHashes;
	static std::vector<size_t> s_LightStateHashes;
	static std::vector<const MaterialAsset*> s_MaterialHashKeys; // the list the hashes were made for
	static std::vector<const Entity*> s_LightHashKeys;
	static uint64_t s_MaterialHashesListVersion = 0;
	static uint64_t s_LightHashesListVersion = 0;

	/**
	 * @enum EntityChange
	 * @brief Flags representing changes to an entity that affect rendering.
	 */
	enum class EntityChange : uint8_t
	{
		None       = 0,
		Active     = 1 << 0,
		Components = 1 << 1,
		CullMode   = 1 << 2,
		LightType  = 1 << 3
	};

	/**
	 * @brief Marks an entity as changed, indicating that its state has been modified.
	 * @param id The unique identifier of the entity.
	 * @param change The type of change that occurred.
	 */
	static void MarkEntityChanged(uint64_t id, EntityChange change)
	{
		s_EntityStates[id] |= static_cast<uint32_t>(change);
		s_Resolve = true;
	}

	namespace
	{
		/**
		 * @brief Drops destroyed entities from s_EntityPointers in one pass. DestroyEntity only queues them: erasing
		 * each one from the vector made unloading tens of thousands of entities quadratic.
		 */
		void CompactEntityPointers()
		{
			if (s_DestroyedEntities.empty())
				return;

			// The pointers are only compared (the entities are gone): sorted for a binary search per entity.
			std::ranges::sort(s_DestroyedEntities);
			std::erase_if(s_EntityPointers, [](const Entity *entityPtr)
			{
				return std::ranges::binary_search(s_DestroyedEntities, entityPtr);
			});
			s_DestroyedEntities.clear();
		}
	}

	/**
	 * @brief Carries change-tracking hashes over to a rebuilt list: entries still listed keep theirs, new ones start at
	 * 0 (so their first hash reads as a change).
	 * @return True when entries were dropped (that is a change the remaining hashes can't show).
	 */
	template <typename T>
	static bool RemapHashes(const std::vector<T*> &current, std::vector<const T*> &keys, std::vector<size_t> &hashes)
	{
		std::unordered_map<const T*, size_t> previous;
		previous.reserve(keys.size());
		for (size_t i = 0; i < keys.size() && i < hashes.size(); ++i)
			previous.emplace(keys[i], hashes[i]);

		std::vector<size_t> next(current.size(), 0);
		size_t kept = 0;
		for (size_t i = 0; i < current.size(); ++i)
		{
			if (const auto it = previous.find(current[i]); it != previous.end())
			{
				next[i] = it->second;
				++kept;
			}
		}

		const bool dropped = kept < keys.size();
		keys.assign(current.begin(), current.end());
		hashes.swap(next);
		return dropped;
	}

	/**
	 * @brief Computes a hash value for a material asset based on its properties and associated textures.
	 * @param material The material asset to hash.
	 * @return The computed hash value.
	 */
	static size_t ComputeMaterialHash(MaterialAsset* material)
	{
		size_t hash = 17; // FNV-1a seed

		// include resource state so async preparation completion triggers an update
		hash = (hash * 31) ^ static_cast<size_t>(material->GetResourceState());

		for (const auto* texture : material->GetTextures())
		{
			hash = (hash * 31) ^ reinterpret_cast<size_t>(texture);

			// include texture's resource state so async texture preparation triggers an update
			if (texture)
			{
				hash = (hash * 31) ^ static_cast<size_t>(texture->GetResourceState());
			}
		}
		for (const float prop : material->GetProperties())
		{
			hash = (hash * 31) ^ std::hash<float>{}(prop);
		}
		return hash;
	}

	/**
	 * @brief Computes a hash value for a light entity based on its properties and the associated entity's state.
	 * @param light The light component to hash.
	 * @param entity The entity associated with the light component.
	 * @return The computed hash value.
	 */
	static size_t ComputeLightHash(Light* light, Entity* entity)
	{
		size_t hash = 17;

		// An inactive light other than the sun isn't uploaded (Renderer::UpdateLights skips it), so only switching
		// it on matters: thousands of X-Plane spill lights sit switched off in daylight.
		if (!entity->GetActive() && light->GetLightType() != LightType::Directional)
			return (hash * 31) ^ static_cast<size_t>(light->GetLightType());


		hash = (hash * 31) ^ std::hash<float>{}(light->GetLightColor().r);
		hash = (hash * 31) ^ std::hash<float>{}(light->GetLightColor().g);
		hash = (hash * 31) ^ std::hash<float>{}(light->GetLightColor().b);
		hash = (hash * 31) ^ std::hash<float>{}(light->GetLightColor().a);
		hash = (hash * 31) ^ std::hash<float>{}(light->GetIntensityWatt());
		hash = (hash * 31) ^ std::hash<float>{}(light->GetRange());
		hash = (hash * 31) ^ std::hash<float>{}(light->GetAngle());
		hash = (hash * 31) ^ std::hash<float>{}(light->GetAreaWidth());
		hash = (hash * 31) ^ std::hash<float>{}(light->GetAreaHeight());
		hash = (hash * 31) ^ static_cast<size_t>(light->GetLightType());
		hash = (hash * 31) ^ static_cast<size_t>(light->GetLightType());
		hash = (hash * 31) ^ static_cast<size_t>(entity->GetActive());

		// world position and forward axis (row-vector world matrix: row 3 translation, row 2 local +Z), so lights
		// that move with an animated parent are noticed too
		const xMath::Matrix world = entity->GetMatrix();
		hash = (hash * 31) ^ std::hash<float>{}(world.m30);
		hash = (hash * 31) ^ std::hash<float>{}(world.m31);
		hash = (hash * 31) ^ std::hash<float>{}(world.m32);
		hash = (hash * 31) ^ std::hash<float>{}(world.m20);
		hash = (hash * 31) ^ std::hash<float>{}(world.m21);
		hash = (hash * 31) ^ std::hash<float>{}(world.m22);

		// the slice matrices follow from the values above; they only reach the GPU for shadowed lights
		const uint32_t sliceCount = light->GetFlag(LightFlags::Shadows) ? light->GetSliceCount() : 0;
		for (uint32_t i = 0; i < sliceCount; i++)
		{
			const Matrix& vp       = light->GetViewProjectionMatrix(i);
			const float* vp_data   = vp.Data();
			for (uint32_t j = 0; j < 16; j++)
			{
				hash = (hash * 31) ^ std::hash<float>{}(vp_data[j]);
			}
		}

		return hash;
	}

	/**
	 * @brief Computes the bounding box for the scene based on the active entities.
	 */
	static void ComputeBoundingBox()
	{
		s_BoundingBox = BoundingBox::UNIT;

		for (Entity* entity : s_EntityPointers)
		{
			if (entity->GetActive())
				{
					if (Renderable* renderable = entity->GetComponent<Renderable>())
					{
						s_BoundingBox.Merge(renderable->GetBoundingBox());
					}
				}
		}
	}


	Scene::Scene(std::string name, bool initialize) : m_Name(std::move(name))
	{
		m_SceneID = UUID();
	}

	void Scene::Init()
	{
		if (!s_ActiveScene)
		{
			s_ActiveScene = CreateScope<Scene>("MainScene", false);
		}

		if (!m_Camera)
		{
			m_Camera = CreateRef<Camera>();
			m_Camera->Init();
		}

		if (!HasCameraEntity())
		{
			Entity cameraEntity = s_ActiveScene->CreateEntity("MainCamera");
			const entt::entity cameraHandle = static_cast<entt::entity>(cameraEntity);

			// +Y is up (Camera::ProcessInput); start above the ground plane with a three-quarter view of the origin
			auto& transform = s_ActiveScene->m_Registry.get<TransformComponent>(cameraHandle);
			constexpr Vec3 kInitialCameraTarget = Vec3(0.0f, 0.5f, 0.0f);
			transform.translation = Vec3(2.5f, 2.5f, -3.5f);
			const Vec3 direction = Normalize(kInitialCameraTarget - transform.translation);
			transform.SetRotationEuler(Vec3(std::asin(direction.y), std::atan2(direction.x, direction.z), 0.0f));

			auto& cameraComponent = s_ActiveScene->m_Registry.emplace<CameraComponent>(cameraHandle);
			cameraComponent.horizontalFov_Rad = 60.0f * xMath::DEG_TO_RAD;
			cameraComponent.nearPlane = 0.1f;
			cameraComponent.farPlane = 10000.0f;
			cameraComponent.projectionType = CameraComponent::ProjectionType::Perspective;
			cameraComponent.isPrimary = true;
			cameraComponent.useJitter = true;
		}

		// Runtime components (Renderable, Light, ...) live on the Entity objects the scene stores, not in the registry.
		CompactEntityPointers();
		const bool hasRenderableEntity = std::ranges::any_of(s_EntityPointers, [](Entity* entity)
		{
			return entity && entity->GetComponent<Renderable>() != nullptr;
		});

		if (!hasRenderableEntity)
		{
			if (const Entity bootstrapHandle = s_ActiveScene->CreateEntity("DefaultSceneMesh"))
			{
				// CreateEntity returns a handle copy; attach runtime components to the stored entity so the renderer
				// (which walks GetEntities()) sees them and the component's entity pointer stays valid.
				Entity* bootstrapEntity = s_EntityStorage.at(static_cast<entt::entity>(bootstrapHandle)).get();

				// unit cube resting on the ground plane
				auto& transform = bootstrapEntity->GetComponent<TransformComponent>();
				transform.translation = Vec3(0.0f, 0.5f, 0.0f);
				transform.scale = Vec3(1.0f, 1.0f, 1.0f);

				if (Renderable* renderable = bootstrapEntity->AddComponent<Renderable>())
				{
					renderable->SetMesh(MeshType::Cube);
					renderable->SetDefaultMaterial();
				}
			}
		}

		Tick();
	}

	void Scene::Shutdown()
	{
		m_Camera.Reset();
		s_EntityStorage.clear();
		s_EntityPointers.clear();
		s_DestroyedEntities.clear();
		s_RenderableEntities.clear();
		s_LightEntities.clear();
		s_RenderableMaterials.clear();
		s_LightStateHashes.clear();
		s_MaterialStateHashes.clear();
		s_LightHashKeys.clear();
		s_MaterialHashKeys.clear();
		s_LightHashesListVersion = 0;
		s_MaterialHashesListVersion = 0;
		MarkStructureChanged();
		s_ActiveScene.reset();
	}

	void Scene::Tick()
	{
	   if (!s_ActiveScene || !m_Camera)
			return;

		auto view = s_ActiveScene->m_Registry.view<TransformComponent, CameraComponent>();

		entt::entity selectedCameraEntity = entt::null;
		view.each([&](const entt::entity entity, TransformComponent&, const CameraComponent& cameraData)
		{
			if (selectedCameraEntity != entt::null)
				return;

			if (cameraData.isPrimary)
			{
				selectedCameraEntity = entity;
				return;
			}

			if (selectedCameraEntity == entt::null)
			{
				selectedCameraEntity = entity;
			}
	   });

		if (selectedCameraEntity == entt::null)
			return;

		auto& transform = view.get<TransformComponent>(selectedCameraEntity);
		auto& cameraData = view.get<CameraComponent>(selectedCameraEntity);

		constexpr float kFixedDeltaTime = 1.0f / 60.0f;
		m_Camera->ProcessInput(transform, kFixedDeltaTime);
		m_Camera->Update(transform, cameraData, Viewport(0.0f, 0.0f,
			static_cast<float>(Window::GetWidth()),
			static_cast<float>(Window::GetHeight())));

	}

	Camera *Scene::GetCamera()
	{
		if (m_Camera)
			return m_Camera.Get();

		return nullptr;
	}

	Scene *Scene::GetActive()
	{
		return s_ActiveScene.get();
	}

	bool Scene::HasCameraEntity()
	{
		if (!s_ActiveScene)
			return false;

		auto view = s_ActiveScene->m_Registry.view<TransformComponent, CameraComponent>();
		return view.size_hint() > 0;
	}

	const std::vector<Entity*> &Scene::GetEntities()
	{
		CompactEntityPointers();
		return s_EntityPointers;
	}

	namespace
	{
		/* @brief Rebuilds the renderable / light / material lists when the structure version has moved. */
		void RefreshEntityLists()
		{
			if (s_EntityListsVersion == s_StructureVersion)
				return;

			CompactEntityPointers();
			s_RenderableEntities.clear();
			s_LightEntities.clear();
			s_RenderableMaterials.clear();
			std::unordered_set<const MaterialAsset*> materials;
			for (Entity *entity : s_EntityPointers)
			{
				if (!entity)
					continue;

				if (Renderable *renderable = entity->GetComponent<Renderable>())
				{
					s_RenderableEntities.push_back(entity);
					if (MaterialAsset *material = renderable->GetMaterialAsset(); material && materials.insert(material).second)
						s_RenderableMaterials.push_back(material);
				}
				if (entity->GetComponent<Light>())
					s_LightEntities.push_back(entity);
			}
			s_EntityListsVersion = s_StructureVersion;
		}
	}

	const std::vector<Entity*> &Scene::GetRenderableEntities()
	{
		RefreshEntityLists();
		return s_RenderableEntities;
	}

	const std::vector<Entity*> &Scene::GetLightEntities()
	{
		RefreshEntityLists();
		return s_LightEntities;
	}

	const std::vector<MaterialAsset*> &Scene::GetRenderableMaterials()
	{
		RefreshEntityLists();
		return s_RenderableMaterials;
	}

	uint64_t Scene::GetStructureVersion()
	{
		return s_StructureVersion;
	}

	void Scene::MarkStructureChanged()
	{
		++s_StructureVersion;
	}

	uint64_t Scene::GetDestroyVersion()
	{
		return s_DestroyVersion;
	}

	bool Scene::SaveToFile(std::string filePath)
	{
		if (filePath.empty())
		{
			return false;
		}

		s_FilePath = std::move(filePath);
		s_SceneName = std::filesystem::path(s_FilePath).stem().string();
		return true;
	}

	bool Scene::LoadFromFile(const std::string& file_path)
	{
		if (file_path.empty())
		{
			return false;
		}

		s_FilePath = file_path;
		s_SceneName = std::filesystem::path(file_path).stem().string();
		return true;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateEntityWithUUID(UUID(), name);
	}

	Entity Scene::CreateEntityWithUUID(const UUID &uuid, const std::string& name)
	{
	  if (!s_ActiveScene)
			return {};

		const entt::entity entityHandle = s_ActiveScene->m_Registry.create();
		s_ActiveScene->m_EntityMap[uuid] = entityHandle;

		s_ActiveScene->m_Registry.emplace<IDComponent>(entityHandle, IDComponent{uuid});
		s_ActiveScene->m_Registry.emplace<TagComponent>(entityHandle, TagComponent{name});
		s_ActiveScene->m_Registry.emplace<RelationshipComponent>(entityHandle);
		s_ActiveScene->m_Registry.emplace<TransformComponent>(entityHandle);

		auto entity = CreateScope<Entity>(entityHandle, s_ActiveScene.get());
		s_EntityPointers.push_back(entity.get());
		s_EntityStorage[entityHandle] = std::move(entity);
		MarkStructureChanged();

		return {entityHandle, s_ActiveScene.get()};
	}

	Entity Scene::GetEntity(const UUID &uuid)
	{
		if (!s_ActiveScene)
			return {};

		if (const auto it = s_ActiveScene->m_EntityMap.find(uuid); it != s_ActiveScene->m_EntityMap.end())
		{
			if (s_ActiveScene->m_Registry.valid(it->second))
			{
				return {it->second, s_ActiveScene.get()};
			}
		}
		SEDX_CORE_ERROR("Entity with UUID {} not found in scene!", static_cast<uint64_t>(uuid));
		return {};
	}

	void Scene::DestroyEntity(const Entity &entity)
	{
		if (!s_ActiveScene || !entity)
			return;

		const entt::entity entityHandle = static_cast<entt::entity>(entity);
		if (!s_ActiveScene->m_Registry.valid(entityHandle))
			return;

		// Children go with their parent (an X-Plane object is a root with animation-group and mesh children).
		// Copy the list: destroying a child edits it, and entt may move components while entities are destroyed.
		if (const RelationshipComponent *relationship = s_ActiveScene->m_Registry.try_get<RelationshipComponent>(entityHandle))
		{
			const std::vector<UUID> children = relationship->pChildren;
			for (const UUID &child : children)
			{
				if (const Entity childEntity = TryGetEntityWithUUID(child); childEntity && static_cast<entt::entity>(childEntity) != entityHandle)
					DestroyEntity(childEntity);
			}
		}

		// Unlink from the parent's child list.
		if (const RelationshipComponent *relationship = s_ActiveScene->m_Registry.try_get<RelationshipComponent>(entityHandle))
		{
			const IDComponent *ownId = s_ActiveScene->m_Registry.try_get<IDComponent>(entityHandle);
			if (const Entity parent = TryGetEntityWithUUID(relationship->pParentHandle); parent && ownId)
			{
				if (RelationshipComponent *parentRelationship = s_ActiveScene->m_Registry.try_get<RelationshipComponent>(static_cast<entt::entity>(parent)))
					std::erase(parentRelationship->pChildren, ownId->pID);
			}
		}

		// The ID lives in the registry; Entity::GetUUID() reads the handle's own (empty) component store.
		if (const IDComponent *id = s_ActiveScene->m_Registry.try_get<IDComponent>(entityHandle))
			s_ActiveScene->m_EntityMap.erase(id->pID);

		if (const auto storageIt = s_EntityStorage.find(entityHandle); storageIt != s_EntityStorage.end())
		{
			s_DestroyedEntities.push_back(storageIt->second.get());
			s_EntityStorage.erase(storageIt);
		}

		s_ActiveScene->m_Registry.destroy(entityHandle);
		MarkStructureChanged();
		++s_DestroyVersion;
	}

	Entity Scene::TryGetEntityWithUUID(const UUID &uuid)
	{
		if (!s_ActiveScene)
			return {};

		if (const auto it = s_ActiveScene->m_EntityMap.find(uuid); it != s_ActiveScene->m_EntityMap.end())
		{
			if (s_ActiveScene->m_Registry.valid(it->second))
			{
			  return {it->second, s_ActiveScene.get()};
			}
		}

		return {};
	}

	entt::entity Scene::TryGetEntityHandle(const UUID &uuid)
	{
		if (!s_ActiveScene)
			return entt::null;

		const auto it = s_ActiveScene->m_EntityMap.find(uuid);
		if (it == s_ActiveScene->m_EntityMap.end() || !s_ActiveScene->m_Registry.valid(it->second))
			return entt::null;
		return it->second;
	}

	Entity *Scene::GetStoredEntity(const Entity &entity)
	{
		if (!s_ActiveScene || !entity)
			return nullptr;

		return GetStoredEntity(static_cast<entt::entity>(entity));
	}

	Entity *Scene::GetStoredEntity(const entt::entity handle)
	{
		if (!s_ActiveScene || handle == entt::null)
			return nullptr;

		const auto it = s_EntityStorage.find(handle);
		return it != s_EntityStorage.end() ? it->second.get() : nullptr;
	}

	float Scene::GetTimeOfDay()
	{
		return DateTime::Instance().Hour() + DateTime::Instance().Minute() / 60.0f +
			   DateTime::Instance().Second() / 3600.0f;
	}

	void Scene::SetTimeOfDay(float timeOfDay)
	{
		DateTime::SetTimeOfDay(timeOfDay);
	}

	std::string &Scene::GetFilePath()
	{
		return s_FilePath;
	}

	const std::string &Scene::GetName()
	{
		return s_SceneName;
	}

	const Vec3& Scene::GetWind()
	{
		return Wind::GetWind();
	}

	void Scene::SetWind(const Vec3 &wind)
	{
		Wind::SetWind(wind);
	}

	Flag Scene::UpdateSceneChanges(const ComponentType entity)
	{
		SEDX_CORE_ASSERT(entity != ComponentType::MaxEnum, "Invalid ComponentType provided to UpdateSceneChanges");
		std::scoped_lock lock(s_EntityAccess_Mutex);
	
		bool changed = false;
		Flag dirty;
	
		switch (entity)
		{
			case ComponentType::Light:
			{
				// Hashes sit in a vector parallel to the cached light list: a map lookup per light per frame was most
				// of the cost with thousands of X-Plane spill lights.
				RefreshEntityLists();
				if (s_LightHashesListVersion != s_EntityListsVersion)
				{
					changed |= RemapHashes(s_LightEntities, s_LightHashKeys, s_LightStateHashes);
					s_LightHashesListVersion = s_EntityListsVersion;
				}
				for (size_t i = 0; i < s_LightEntities.size(); ++i)
				{
					Entity* ent = s_LightEntities[i];
					const size_t currentHash = ComputeLightHash(ent->GetComponent<Light>(), ent);
					if (s_LightStateHashes[i] != currentHash)
					{
						s_LightStateHashes[i] = currentHash;
						changed = true;
					}
				}
				break;
			}
	
			case ComponentType::Material:
			{
				// Each distinct material once: thousands of renderables share far fewer materials.
				RefreshEntityLists();
				if (s_MaterialHashesListVersion != s_EntityListsVersion)
				{
					changed |= RemapHashes(s_RenderableMaterials, s_MaterialHashKeys, s_MaterialStateHashes);
					s_MaterialHashesListVersion = s_EntityListsVersion;
				}
				for (size_t i = 0; i < s_RenderableMaterials.size(); ++i)
				{
					const size_t currentHash = ComputeMaterialHash(s_RenderableMaterials[i]);
					if (s_MaterialStateHashes[i] != currentHash)
					{
						s_MaterialStateHashes[i] = currentHash;
						changed = true;
					}
				}
				break;
			}
	
			case ComponentType::Camera:
			case ComponentType::Renderable:
			case ComponentType::Spline:
			case ComponentType::Terrain:
			case ComponentType::Volume:
			case ComponentType::Script:
			case ComponentType::Plugin:
			case ComponentType::Submesh:
			case ComponentType::ParticleSystem:
				SEDX_CORE_TRACE_TAG("Scene", "Scene component type not handled");
				break;
	
			case ComponentType::MaxEnum:
				SEDX_CORE_ERROR_TAG("Scene", "Invalid ComponentType provided to UpdateSceneChanges");
				break;
		}
	
		if (changed)
			dirty.SetDirty();
	
		return dirty;
	}

	/*
	SceneAsset::SceneAsset()
	{
		type = ObjectType::SceneAsset;
		sceneName = "New Scene";
	}

	void SceneAsset::Serialize(Serializer &s)
	{
		s("uuid", uuid);
		s("name", name);
		s("type", (uint8_t&)type);
		s("path", scenePath);
		s("ambientLightColor", ambientLightColor);
		s("ambientLight", ambientLight);
		s("aoSamples", aoSamples);
		s("lightSamples", lightSamples);
		s("aoMin", aoMin);
		s("aoMax", aoMax);
		s("exposure", exposure);
		//s("shadowType", (uint8_t&)shadowType);
		s("shadowResolution", shadowResolution);
		s("camSpeed", camSpeed);
		s("zoomSpeed", zoomSpeed);
		s("rotationSpeed", rotationSpeed);
		s("autoOrbit", autoOrbit);
		s.Node("mainCamera", mainCamera, this);
		s.VectorRef("nodes", nodes);
	}

	void SceneAsset::Load(const std::string &path)
	{
		scenePath = path;
		isLoaded = true;
	}

	void SceneAsset::Unload()
	{
		nodes.clear();
		mainCamera = nullptr;
		isLoaded = false;
	}

	bool SceneAsset::IsLoaded() const
	{
		return isLoaded;
	}

	const std::string &SceneAsset::GetPath() const
	{
		return scenePath;
	}

	const std::string &SceneAsset::GetName() const
	{
		return sceneName;
	}

	void SceneAsset::SetName(const std::string &name)
	{
		sceneName = name;
		this->name = name;
	}

	void SceneAsset::DeleteRecursive(const Ref<Node> &node)
	{
		/// Remove all children recursively
		for (int i = static_cast<int>(node->children.size()) - 1; i >= 0; i--)
		{
			DeleteRecursive(node->children[i]);
		}
		
		/// Remove the node from its parent's children list
		if (node->parent)
		{
			auto &children = node->parent->children;
			children.erase(std::ranges::remove(children, node).begin(), children.end());
		}
		
		/// Remove from the scene's nodes list if it's a top-level node
		if (const auto it = std::ranges::find(nodes, node); it != nodes.end())
		{
			nodes.erase(it);
		}
	}

	Scene::Scene(const std::string &name, bool isEditorScene, bool initialize) : m_SceneID()
	{
	}

	Scene::~Scene()
	{
	}
	*/

} // namespace SceneryEditorX

// ---------------------------------------------------------
