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
 * editor_scene.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <string>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/scene/components/component_sets.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorScene
{
	/**
	 * Editor access to the active scene's entities.
	 *
	 * Struct components (IDComponent, TagComponent, TransformComponent, CameraComponent, ...) live in the scene's
	 * entt registry; Entity's own struct-component store is not populated by Scene::CreateEntity, so editor code
	 * reads and writes those through the registry. Runtime components (Renderable, ...) stay on the Entity objects
	 * owned by the scene (Scene::GetEntities()). Entities are addressed by their UUID value (0 = none).
	 */

	/* @brief The active scene's registry, or nullptr when no scene is loaded. */
	inline entt::registry *GetRegistry()
	{
		Scene *scene = Scene::GetActive();
		return scene ? &scene->GetRegistry() : nullptr;
	}

	/* @brief Registry component T of an entity handle, or nullptr. */
	template<typename T>
	T *TryGet(const entt::entity handle)
	{
		entt::registry *registry = GetRegistry();
		return (registry && registry->valid(handle)) ? registry->try_get<T>(handle) : nullptr;
	}

	/* @brief True when the entity handle has registry component T. */
	template<typename T>
	bool Has(const entt::entity handle)
	{
		return TryGet<T>(handle) != nullptr;
	}

	/* @brief UUID value of an entity handle (0 when invalid). */
	inline uint64_t GetID(const entt::entity handle)
	{
		const IDComponent *id = TryGet<IDComponent>(handle);
		return id ? static_cast<uint64_t>(id->pID) : 0;
	}

	/* @brief Registry handle for a UUID value (entt::null when not found). */
	inline entt::entity FindHandle(const uint64_t id)
	{
		if (id == 0)
			return entt::null;

		return Scene::TryGetEntityHandle(UUID(id));
	}

	/* @brief The scene-owned Entity object for a handle (where runtime components such as Renderable live). */
	inline Entity *FindEntity(const entt::entity handle)
	{
		return Scene::GetStoredEntity(handle);
	}

	/* @brief Display name of an entity (its TagComponent, or "Entity"). */
	inline const char *GetName(const entt::entity handle)
	{
		const TagComponent *tag = TryGet<TagComponent>(handle);
		return (tag && !tag->pTag.empty()) ? tag->pTag.c_str() : "Entity";
	}

	/* @brief Creates an empty entity and returns its UUID value (0 on failure). */
	inline uint64_t CreateEmpty(const std::string &name)
	{
		const Entity entity = Scene::CreateEntity(name);
		return entity ? GetID(static_cast<entt::entity>(entity)) : 0;
	}

	/* @brief Destroys the entity with the given UUID value, if it exists. */
	inline void Destroy(const uint64_t id)
	{
		const Entity entity = Scene::TryGetEntityWithUUID(UUID(id));
		if (entity)
			Scene::DestroyEntity(entity);
	}

}

// -------------------------------------------------------
