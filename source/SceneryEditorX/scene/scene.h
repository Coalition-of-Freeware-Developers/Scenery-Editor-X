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
	class Light;
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
		explicit Scene(std::string name = "UntitledProject", bool initialize = true);
		static void Init();
		static void Shutdown();
		static void Tick();

		static Camera *GetCamera();
		static bool HasCameraEntity();
		static Light *GetDirectionalLight() { return nullptr; }
		static std::vector<Entity*> GetEntities();

		static bool SaveToFile(std::string filePath);
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
		 * @brief 
		 * @return The current time of day.
		 */
		static float GetTimeOfDay();


		/**
		 * @brief 
		 * @param timeOfDay 
		 */
		static void SetTimeOfDay(float timeOfDay);

		/**
		 * @brief 
		 * @return 
		 */
		static std::string& GetFilePath();

		/**
		 * @brief 
		 * @return 
		 */
		static const std::string& GetName();

		/**
		 * @brief 
		 * @return 
		 */
		static const Vec3 &GetWind();

		/**
		 * @brief 
		 * @param wind 
		 */
		static void SetWind(const Vec3& wind);

	private:
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

	/*
	class Scene : public Asset
	{
	public:
		explicit Scene(const std::string &name = "UntitledProject", bool isEditorScene = false, bool initialize = true);
		virtual ~Scene() override;

		//virtual void Serialize(Serializer &s) override;
		virtual void Load(const std::string& path) override;
		virtual void Unload() override;
		bool IsLoaded() const { return isLoaded; }
		const std::string& GetPath() const { return scenePath; }
		const std::string& GetName() const { return m_Name; }
		virtual void SetName(const std::string& name) override;
		void Add(const Ref<CameraNode> & ref);
		UUID GetUUID() const { return m_SceneID; }
		static Ref<Scene> CreateEmpty();
		static Ref<Scene> GetScene(UUID uuid);

		float GetTimeScale() const { return m_TimeScale; }
		void SetTimeScale(float timeScale) { m_TimeScale = timeScale; }
		static AssetType GetStaticType() { return AssetType::Scene; }
		virtual ObjectType GetAssetType() const override { return static_cast<ObjectType>(GetStaticType()); }

		uint32_t GetViewportWidth() const { return m_ViewportRight - m_ViewportLeft; }
		uint32_t GetViewportHeight() const { return m_ViewportBottom - m_ViewportTop; }
		
		//const EntityMap& GetEntityMap() const { return m_EntityIDMap; }
		std::pair<std::unordered_set<AssetHandle>, std::unordered_set<AssetHandle>> GetAssetList(); // returns assetList, missingAssetList

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

		/// return entity with id as specified. entity is expected to exist (runtime error if it doesn't)
		Entity GetEntityWithUUID(UUID id) const;
		
		/// return entity with id as specified, or empty entity if it cannot be found - caller must check
		Entity TryGetEntityWithUUID(UUID id) const;

		/// return entity with tag as specified, or empty entity if it cannot be found - caller must check
		Entity TryGetEntityWithTag(const std::string& tag);

		/// return descendant entity with tag as specified, or empty entity if it cannot be found - caller must check
		/// descendant could be immediate child, or deeper in the hierarchy
		Entity TryGetDescendantEntityWithTag(Entity entity, const std::string& tag) const;

		/// return descendant entity that has component type specified in template, or null entity if it cannot be found - caller must check
		template<typename T>
		Entity TryGetDescendantEntityWithComponent(Entity entity) const
		{
			if (entity.TryGetComponent<T>())
				return entity;

			for (const auto childId : entity.Children())
			{
				if (Entity child = TryGetEntityWithUUID(childId))
				{
					if (Entity descendant = TryGetDescendantEntityWithComponent<T>(child))
						return descendant;
				}
			}

			return {};
		}

		void ConvertToLocalSpace(Entity entity);
		void ConvertToWorldSpace(Entity entity);
		Mat4 GetWorldSpaceTransformMatrix(Entity entity);
		void SetWorldSpaceTransformMatrix(Entity entity, const Mat4& transform);
		TransformComponent GetWorldSpaceTransform(Entity entity);
		void SetWorldSpaceTransform(Entity entity, const TransformComponent& transform);

		void ParentEntity(Entity entity, Entity parent);
		void UnparentEntity(Entity entity, bool convertToWorldSpace = true);

		void CopyTo(Ref<Scene>& target);

		/**
		 * @brief Copies a component from the source entity to the destination entity if it exists.
		 *
		 * @tparam TComponent 
		 * @param dst 
		 * @param dstRegistry 
		 * @param src 
		 *
		 * @note - Used to have ".has<>" but has been replaced in Entt 4.0 with ".all_of<>()"
		 #1#
		template<typename TComponent>
		void CopyComponentIfExists(entt::entity dst, entt::registry& dstRegistry, entt::entity src)
		{
			if (m_Registry.all_of<TComponent>(src))
			{
				auto &srcComponent = m_Registry.get<TComponent>(src);
				dstRegistry.emplace_or_replace<TComponent>(dst, srcComponent);
			}
		}

		template<typename TComponent>
		static void CopyComponentFromScene(Entity dst, Ref<Scene> dstScene, Entity src, Ref<Scene> srcScene)
		{
			srcScene->CopyComponentIfExists<TComponent>((entt::entity)dst, dstScene->m_Registry, (entt::entity)src);
		}

		void DuplicateAnimationInstance(Entity dst, Entity src);


		//std::vector<Ref<GraphNode>> nodes;
		Vec3 ambientLightColor = Vec3(1);
		float ambientLight = 0.01f;
		int aoSamples = 4;
		int lightSamples = 2;
		float aoMin = 0.0001f;
		float aoMax = 1.0000f;
		float exposure = 2.0f;
		uint32_t shadowResolution = 1024;
		float camSpeed = 0.01f;
		float zoomSpeed = 1.0f;
		float rotationSpeed = 0.3f;
		bool autoOrbit = false;
		Ref<CameraNode> mainCamera;
	private:
		UUID m_SceneID;
		std::size_t m_LastSerializeHash = 0; /// used by auto-save to determine if scene has changed
		entt::entity m_SceneEntity = entt::null;
		entt::registry m_Registry;
		std::string scenePath;

		std::function<void(AssetHandle)> m_OnSceneTransitionCallback;
		//std::function<void(Entity)> m_OnEntityDestroyedCallback;
		bool isLoaded = false;
		std::string m_Name;
		bool m_IsEditorScene = false;
		uint32_t m_ViewportTop = 0;
		uint32_t m_ViewportLeft = 0;
		uint32_t m_ViewportRight = 0;
		uint32_t m_ViewportBottom = 0;

		//EntityMap m_EntityIDMap;
		DirectionalLightComponent m_Light;
		float m_LightMultiplier = 0.3f;
		SkyLightComponent m_LightEnvironment;

		Ref<Environment> m_Environment;
		float m_EnvironmentIntensity = 0.0f;

		std::vector<std::function<void()>> m_PostUpdateQueue;

		float m_SkyboxLod = 1.0f;
		bool m_IsPlaying = false;
		bool m_ShouldSimulate = false;
		float m_TimeScale = 1.0f;


		friend class Entity;
		friend class Prefab;
		friend class SceneRenderer;
		//friend class SceneSerializer;
		//friend class PrefabSerializer;
		friend class SceneHierarchyPanel;
		friend class ECSDebugPanel;
	};

	/*
	struct Serializer;
	class EditorConfig;
	
	// -------------------------------------------------------
	
	class SceneAsset : public Asset
	{
	public:
		SceneAsset();
		virtual void Serialize(Serializer &s) override;
		virtual void Load(const std::string &path) override;
		virtual void Unload() override;
		bool IsLoaded() const;
		const std::string &GetPath() const;
		const std::string &GetName() const;
		virtual void SetName(const std::string &name) override;

		std::vector<Ref<Node>> nodes;
		Vec3 ambientLightColor = Vec3(1);
		float ambientLight = 0.01f;
		int aoSamples = 4;
		int lightSamples = 2;
		float aoMin = 0.0001f;
		float aoMax = 1.0000f;
		float exposure = 2.0f;
		//ShadowType shadowType = ShadowType::ShadowRayTraced;
		uint32_t shadowResolution = 1024;

		float camSpeed = 0.01f;
		float zoomSpeed = 1.0f;
		float rotationSpeed = 0.3f;
		bool autoOrbit = false;
		Ref<CameraNode> mainCamera;

		template <typename T>
		Ref<T> Add()
		{
			Ref<T> node = std::make_shared<T>();
			nodes.push_back(node);
			return node;
		}

		void Add(const Ref<Node> &node)
		{
			nodes.push_back(node);
		}

		void DeleteRecursive(const Ref<Node> &node);

		template<typename T>
		Ref<T> GetAsset(uint32_t id)
		{
			// TODO: Search recursively
			for (auto& node : nodes)
			{
				if (node->uuid == id)
				{
					return std::dynamic_pointer_cast<T>(node);
				}
			}
			return {};
		}

		template<typename T>
		void GetAll(ObjectType type, std::vector<Ref<T>>& all)
		{
			for (auto& node : nodes)
			{
				if (node->type == type)
				{
					all.emplace_back(std::dynamic_pointer_cast<T>(node));
				}
				node->GetAll(type, all);
			}
		}

		template<typename T>
		std::vector<Ref<T>> GetAll(ObjectType type)
		{
			std::vector<Ref<T>> all;
			for (auto& node : nodes)
			{
				if (node->type == type)
				{
					all.emplace_back(std::dynamic_pointer_cast<T>(node));
				}
				node->GetAll(type, all);
			}
			return all;
		}

		void UpdateParents()
		{
			for (auto& node : nodes)
			{
				node->parent = {};
				Node::UpdateChildrenParent(node);
			}
		}

	private:
		friend class AssetManager;
		std::string scenePath;
		std::string sceneName;
		bool isLoaded = false;
	};
*/
	
}


// ---------------------------------------------------------
