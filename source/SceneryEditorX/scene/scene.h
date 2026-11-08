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
 * scene.h
 * -------------------------------------------------------
 * Created: 11/4/2025
 * -------------------------------------------------------
 */
// ReSharper disable CppInconsistentNaming
#pragma once
#include "camera.h"
#include <unordered_map>
#include <vector>
#include <SceneryEditorX/core/identifiers/uuid.h>
#include <entt/src/entt/entt.hpp>
//#include "SceneryEditorX/asset/asset.h"
//#include "SceneryEditorX/asset/asset_types.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Entity;
	struct EntityRuntime;
	class Light;
	class MaterialAsset;
	class Camera;

	/**
	 * @struct WorldMetadata
	 * @brief Structure to hold metadata about a world without fully loading it.
	 */
	struct WorldMetadata
	{
		std::string file_path;
		std::string name;
		std::string description;
	};

	/**
	 * @class Scene
	 * @brief Class representing a scene in the editor.
	 */
	class Scene : public RefCounted
	{
	public:
		/**
		 * @brief Constructs a new Scene object.
		 * @param name The name of the scene.
		 * @param initialize Whether to initialize the scene immediately.
		 */
		explicit Scene(std::string name = "UntitledProject", bool initialize = true);
		static void Init();
		static void Shutdown();
		static void Tick();

		static Camera *GetCamera();
		static bool HasCameraEntity();

		/* @brief The active scene, or nullptr before Init() / after Shutdown(). */
		static Scene *GetActive();

		/* @brief The ECS registry holding this scene's components (IDComponent, TagComponent, TransformComponent, ...). */
		entt::registry &GetRegistry() { return m_Registry; }
		static Light *GetDirectionalLight() { return nullptr; }

		/* @brief Every scene-owned entity, in creation order. Don't create or destroy entities while iterating it. */
		static const std::vector<Entity*> &GetEntities();

		/**
		 * @brief The stored entities that carry a Renderable / a Light. Cached: rebuilt when the structure version
		 * (GetStructureVersion) has moved, so per-frame passes don't walk every entity.
		 */
		static const std::vector<Entity*> &GetRenderableEntities();
		static const std::vector<Entity*> &GetLightEntities();

		/* @brief The distinct materials of GetRenderableEntities() (cached the same way). */
		static const std::vector<MaterialAsset*> &GetRenderableMaterials();

		/**
		 * @brief Bumped when entities are created or destroyed, runtime components are added or removed, or a
		 * renderable's mesh / material changes. Caches of entity pointers rebuild when it moves.
		 */
		static uint64_t GetStructureVersion();
		static void MarkStructureChanged();

		/**
		 * @brief Registers a reader of the render state that changes without a structure change: entities switched on or
		 * off (Entity::SetActive) and renderable flags (Renderable::SetFlag). A per-frame pass that keeps its own copy of
		 * that state (GpuScene, the renderer's CPU draw list) then reads only what changed, not every entity each frame.
		 * @return The reader's id for TakeRenderStateChanges(). Its first take asks for a full read.
		 */
		static uint32_t AddRenderStateReader();

		/**
		 * @brief The entities whose render state changed since the reader's last take, in order. An entity can be listed
		 * more than once, or be destroyed since: compare with what you kept and skip handles that are no longer live.
		 * @return False (and no changes) when the reader has to re-read every entity instead: its first take, after
		 * Shutdown(), or when it fell so far behind that its changes were dropped.
		 */
		static bool TakeRenderStateChanges(uint32_t reader, std::vector<entt::entity> &changes);

		/* @brief Queues an entity for every reader (Entity::SetActive, Renderable::SetFlag call it on a change). Any thread. */
		static void MarkRenderStateChanged(entt::entity entity);

		/* @brief Bumped only when an entity is destroyed: caches of Entity pointers revalidate when it moves. */
		static uint64_t GetDestroyVersion();

		/**
		 * @brief Saves the current scene to a file.
		 * @param filePath The path to the file where the scene will be saved.
		 * @return True if the scene was successfully saved, false otherwise.
		 */
		static bool SaveToFile(std::string filePath);

		/**
		 * @brief Loads a scene from a file.
		 * @param file_path The path to the file from which the scene will be loaded.
		 * @return True if the scene was successfully loaded, false otherwise.
		 */
		static bool LoadFromFile(const std::string& file_path);

		/**
		 * @brief Creates a new entity in the scene.
		 * @param name The name of the entity.
		 * @return The created entity.
		 */
		static Entity CreateEntity(const std::string& name = "Empty Entity");

		/**
		 * @brief Creates a new entity in the scene with a specific UUID.
		 * @param uuid The UUID of the entity.
		 * @param name The name of the entity.
		 * @return The created entity.
		 */
		static Entity CreateEntityWithUUID(const UUID &uuid, const std::string& name = "Empty Entity");

		/**
		 * @brief Retrieves an entity from the scene by its UUID.
		 * @param uuid The UUID of the entity.
		 * @return The entity with the specified UUID.
		 */
		static Entity GetEntity(const UUID &uuid);

		/**
		 * @brief Destroys an entity in the scene.
		 * @param entity The entity to be destroyed.
		 */
		static void DestroyEntity(const Entity &entity);

		/**
		 * @brief Tries to retrieve an entity from the scene by its UUID.
		 * @param uuid The UUID of the entity.
		 * @return The entity with the specified UUID, or a null entity if not found.
		 */
		static Entity TryGetEntityWithUUID(const UUID &uuid);

		/**
		 * @brief Like TryGetEntityWithUUID(), returning only the registry handle.
		 * @return The registry handle, or entt::null when no live entity has the UUID.
		 */
		static entt::entity TryGetEntityHandle(const UUID &uuid);

		/**
		 * @brief The entity's stored handle (the one in its EntityRuntime record): a fixed address until the entity is
		 * destroyed, the owner runtime components (Renderable, ...) point to and what GetEntities() lists.
		 * @param entity A handle returned by CreateEntity() / TryGetEntityWithUUID().
		 * @return The stored entity, or nullptr when the handle isn't part of the active scene.
		 */
		static Entity *GetStoredEntity(const Entity &entity);
		static Entity *GetStoredEntity(entt::entity handle);

		/**
		 * @brief Retrieves all entities with the specified components.
		 * @tparam Components The components to filter by.
		 * @return A view of all entities with the specified components.
		 */
		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

		/**
		 * @brief Retrieves the current time of day in the scene.
		 * @return The current time of day.
		 */
		static float GetTimeOfDay();


		/**
		 * @brief Sets the current time of day in the scene.
		 * @param timeOfDay The time of day to set.
		 */
		static void SetTimeOfDay(float timeOfDay);

		/**
		 * @brief Retrieves the file path of the current scene.
		 * @return The file path of the current scene.
		 */
		static std::string& GetFilePath();

		/**
		 * @brief Retrieves the name of the current scene.
		 * @return The name of the current scene.
		 */
		static const std::string& GetName();

		/**
		 * @brief Retrieves the current wind vector in the scene.
		 * @return The current wind vector.
		 */
		static const Vec3 &GetWind();

		/**
		 * @brief Sets the current wind vector in the scene.
		 * @param wind The wind vector to set.
		 */
		static void SetWind(const Vec3& wind);

		/**
		 * @brief Updates the scene based on changes to an entity's components.
		 * @return A flag indicating the changes that were applied.
		 */
		static Flag UpdateSceneChanges(ComponentType entity);

	private:
		/**
		 * @brief The runtime record of a live entity of the active scene, by registry handle (an index lookup).
		 * @return The record, or nullptr for entt::null, another scene or a destroyed entity (the version differs).
		 */
		static EntityRuntime *FindRuntime(entt::entity handle, const Scene *scene);

		static Ref<Camera> m_Camera;

		UUID m_SceneID;
		std::string m_Name;
		std::string m_ScenePath;

		bool m_IsLoaded				= false;
		bool m_IsEditorScene		= false;
		uint32_t m_ViewportTop		= 0;
		uint32_t m_ViewportLeft		= 0;
		uint32_t m_ViewportRight	= 0;
		uint32_t m_ViewportBottom	= 0;

		entt::registry m_Registry; // <-- THE ACTUAL DATA STORAGE
		std::unordered_map<UUID, entt::entity> m_EntityMap; // Fast lookup

		friend class Entity; // Allows Entity handle to access m_Registry
	};

	/*
	class Environment : public Asset
	{
	public:
		Ref<TextureCube> RadianceMap;
		Ref<TextureCube> IrradianceMap;

		Environment() = default;
		Environment(const Ref<TextureCube>& radianceMap, const Ref<TextureCube>& irradianceMap) : RadianceMap(radianceMap), IrradianceMap(irradianceMap) {}

		static ObjectType GetStaticType() { return ObjectType::EnvMap; }
		virtual ObjectType GetAssetType() const override { return GetStaticType(); }

	};
	*/

	
}


// ---------------------------------------------------------
