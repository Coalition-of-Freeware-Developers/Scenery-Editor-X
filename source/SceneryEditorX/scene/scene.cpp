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
#include "SceneryEditorX/core/time/date_time.h"
#include "components/component_sets.h"
#include "components/wind.h"

#include <algorithm>
#include <filesystem>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/scene/camera.h>
#include <entt/entity/fwd.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{

	Ref<Camera> Scene::m_Camera = nullptr;
	Scope<Scene> s_ActiveScene = nullptr;
	std::unordered_map<entt::entity, Scope<Entity>> s_EntityStorage;
	std::vector<Entity*> s_EntityPointers;
	std::string s_FilePath;
	std::string s_SceneName; // cached to avoid per-frame allocation
	std::string s_SceneDescription;

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

			auto& transform = s_ActiveScene->m_Registry.get<TransformComponent>(cameraHandle);
			constexpr Vec3 kInitialCameraTarget = Vec3(0.0f, 0.0f, 0.0f);
			transform.translation = Vec3(0.0f, -5.0f, -5.0f);
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

		Tick();

	}

	void Scene::Shutdown()
	{
		m_Camera.Reset();
		s_EntityStorage.clear();
		s_EntityPointers.clear();
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
		{
			return m_Camera.Get();
		}

		return nullptr;
	}

	bool Scene::HasCameraEntity()
	{
		if (!s_ActiveScene)
			return false;

		auto view = s_ActiveScene->m_Registry.view<TransformComponent, CameraComponent>();
		return view.size_hint() > 0;
	}

	std::vector<Entity*> Scene::GetEntities()
	{
		return s_EntityPointers;
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

		const UUID entityId = entity.GetUUID();
		const entt::entity entityHandle = static_cast<entt::entity>(entity);
		s_ActiveScene->m_EntityMap.erase(entityId);

		if (const auto storageIt = s_EntityStorage.find(entityHandle); storageIt != s_EntityStorage.end())
		{
			const Entity* entityPtr = storageIt->second.get();
			s_EntityPointers.erase(std::ranges::remove(s_EntityPointers, entityPtr).begin(), s_EntityPointers.end());
			s_EntityStorage.erase(storageIt);
		}

		s_ActiveScene->m_Registry.destroy(entityHandle);
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
