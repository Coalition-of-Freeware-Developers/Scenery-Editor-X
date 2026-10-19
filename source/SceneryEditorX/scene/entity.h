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
 * entity.h
 * -------------------------------------------------------
 * Created: 11/8/2025
 * -------------------------------------------------------
 */
// ReSharper disable CppInconsistentNaming
#pragma once
#include "scene.h"
#include <array>
#include <concepts>
#include <string>
#include <type_traits>
#include <SceneryEditorX/scene/components/component.h>
#include <entt/src/entt/entt.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EntityRuntime;

	/**
	 * @class Entity
	 * @brief A handle to a scene entity: its registry handle, its scene and (when known) its runtime record.
	 *
	 * Trivially copyable and allocation-free, so build and pass it by value. Struct components (IDComponent,
	 * TagComponent, TransformComponent, ...) live in the scene registry. Runtime components (Renderable, Light, ...),
	 * the active flag and the previous world matrix live in the entity's EntityRuntime, which the scene owns; every
	 * handle to the entity reaches the same record, so a copy sees and changes the same state.
	 *
	 * Handles the scene hands out (CreateEntity, TryGetEntityWithUUID, GetParent, ...) carry the record's address;
	 * one built from a bare registry handle (Entity(handle, scene)) looks the record up on each runtime-component
	 * access. Scene::GetStoredEntity() / GetEntities() return the record's own handle: its address stays put until
	 * the entity is destroyed, which is why runtime components keep it as their owner.
	 */
	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene) : m_EntityHandle(handle), m_Scene(scene) {}

		void Start();
		void Stop();
		void Tick();

		/**
		 * @brief Whether the entity takes part in the frame (the renderer skips inactive entities).
		 * @return The flag SetActive() set on any handle to this entity; false when it isn't a live scene entity.
		 */
		[[nodiscard]] bool GetActive() const;

		/**
		 * @brief Enables or disables the entity (for example animation hide / show and LOD switching).
		 * @param active New state.
		 */
		void SetActive(bool active);

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] bool IsValid() const;

		/**
		 * @brief 
		 * @return  
		 */
		[[nodiscard]] xMath::Matrix GetMatrix() const; // world transform (registry TransformComponent + parent chain), row-vector

		/* @brief The local transform of a TransformComponent as a row-vector matrix (what GetMatrix() composes). */
		static xMath::Matrix ComposeLocalMatrix(const TransformComponent &transform);

		/**
		 * @brief Stores the world transform the renderer used for this entity this frame (motion vectors).
		 */
		void SetMatrixPrevious(const xMath::Matrix& m);

		/**
		 * @brief World transform the renderer used for this entity last frame (motion vectors).
		 * @return The stored previous matrix, or the current one before the first frame was rendered.
		 */
		[[nodiscard]] xMath::Matrix GetMatrixPrevious() const;

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] xMath::Vec3 GetPosition() const;

		/**
		 * @brief 
		 * @param vector3 
		 * @return 
		 */
		xMath::Vec3 SetPosition(xMath::Vec3 vector3);

		/**
		 * @brief Gets this entity's local-space position.
		 * @return Local translation.
		 */
		[[nodiscard]] xMath::Vec3 GetPositionLocal() const;

		/**
		 * @brief Sets this entity's local-space position.
		 * @param vector3 Local translation to apply.
		 * @return Applied local translation.
		 */
		xMath::Vec3 SetPositionLocal(xMath::Vec3 vector3);

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] xMath::Vec3 GetRotation() const;

		/**
		 * @brief 
		 * @param vector3 
		 * @return 
		 */
		xMath::Vec3 SetRotation(xMath::Vec3 vector3);

		/**
		 * @brief Gets this entity's local-space euler rotation (radians).
		 * @return Local rotation in radians.
		 */
		[[nodiscard]] xMath::Vec3 GetRotationLocal() const;

		/**
		 * @brief Sets this entity's local-space euler rotation (radians).
		 * @param vector3 Local rotation in radians.
		 * @return Applied local rotation in radians.
		 */
		xMath::Vec3 SetRotationLocal(xMath::Vec3 vector3);

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] xMath::Vec3 GetScale() const;

		/**
		 * @brief 
		 * @param vector3 
		 * @return 
		 */
		xMath::Vec3 SetScale(xMath::Vec3 vector3);

		/**
		 * @brief Gets this entity's local-space scale.
		 * @return Local scale.
		 */
		[[nodiscard]] xMath::Vec3 GetScaleLocal() const;

		/**
		 * @brief Sets this entity's local-space scale.
		 * @param vector3 Local scale to apply.
		 * @return Applied local scale.
		 */
		xMath::Vec3 SetScaleLocal(xMath::Vec3 vector3);

		/**
		 * @brief Gets this entity's local-space pivot point.
		 * @return Local pivot point.
		 */
		[[nodiscard]] xMath::Vec3 GetPivotPoint() const;

		/**
		 * @brief Sets this entity's local-space pivot point.
		 * @param vector3 Local pivot point to apply.
		 */
		void SetPivotPoint(const Vec3& vector3);

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] Vec3 GetForward() const;

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] Vec3 GetUp() const;

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] Vec3 GetDown() const;

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] Vec3 GetLeft() const;

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] Vec3 GetRight() const;

		/**
		 * @brief The runtime component of a type, or nullptr.
		 * @param Type The component type.
		 * @return The component in the entity's runtime record, or nullptr (also when the entity isn't live).
		 */
		[[nodiscard]] Component* GetComponentByType(ComponentType Type) const;

		/**
		 * @brief 
		 * @param Type 
		 * @return 
		 */
		Component* AddComponentByType(ComponentType Type);

		/**
		 * @brief 
		 * @param Type 
		 */
		void RemoveComponentByType(ComponentType Type);

		// Adds a component by ComponentType enum value
		/**
		 * @brief 
		 * @param type 
		 * @return 
		 */
		Component* AddComponent(ComponentType type);

		// ---- Struct component access (non-Component subclasses) - returns T& ----

		/**
		 * @brief Returns a reference to a struct component of type T.
		 * @tparam T A non-Component struct type (e.g. IDComponent, TagComponent).
		 * @return Reference to the stored component.
		 */
		template<typename T>
		T &GetComponent() requires(!ComponentTypeResolver<T>::IS_REGISTERED)
		{
			entt::registry* registry = GetRegistry();
			SEDX_CORE_ASSERT(registry && registry->all_of<T>(m_EntityHandle), "Entity does not have component of this type");
			return registry->get<T>(m_EntityHandle);
		}

		/**
		 * @brief Returns a const reference to a struct component of type T.
		 * @tparam T A non-Component struct type.
		 * @return Const reference to the stored component.
		 */
		template<typename T>
		const T &GetComponent() const requires(!ComponentTypeResolver<T>::IS_REGISTERED)
		{
			const entt::registry* registry = GetRegistry();
			SEDX_CORE_ASSERT(registry && registry->all_of<T>(m_EntityHandle), "Entity does not have component of this type");
			return registry->get<T>(m_EntityHandle);
		}

		// ---- Runtime component access (Component subclasses) - returns T* ----

		/**
		 * @brief Returns a pointer to a runtime component of type T, or nullptr if absent.
		 * @tparam T A class derived from Component (e.g. CameraComponent, LightComponent).
		 * @return Pointer to the component, or nullptr.
		 */
		template<typename T>
		T *GetComponent() requires(ComponentTypeResolver<T>::IS_REGISTERED && std::derived_from<T, Component>)
		{
			return static_cast<T*>(GetComponentByType(Component::TypeToEnum<T>()));
		}

		template<typename T>
		T *GetComponent() requires(ComponentTypeResolver<T>::IS_REGISTERED && !std::derived_from<T, Component>)
		{
			return nullptr;
		}

		/**
		 * @brief Returns a const pointer to a runtime component of type T, or nullptr if absent.
		 * @tparam T A class derived from Component.
		 * @return Const pointer to the component, or nullptr.
		 */
		template<typename T>
		const T *GetComponent() const requires(ComponentTypeResolver<T>::IS_REGISTERED && std::derived_from<T, Component>)
		{
			return static_cast<const T*>(GetComponentByType(Component::TypeToEnum<T>()));
		}

		template<typename T>
		const T *GetComponent() const requires(ComponentTypeResolver<T>::IS_REGISTERED && !std::derived_from<T, Component>)
		{
			return nullptr;
		}

		// ---- AddComponent ----

		// TODO: Add full component cloning support, including for struct components. This will likely require a virtual Clone method on Component, and some way to clone struct components (e.g. via a registered clone function or by requiring them to be copyable).
		/**
		 * @brief Copies all components from this entity to a new entity in the same scene. Returns pointer to the new entity.
		 * @tparam T 
		 * @return 
		 */
		template<typename T>
		Entity *Clone(T)
		{
			return nullptr;
		}

		// ---- Operators & Utilities ----

		/**
		 * @brief 
		 */
		operator bool() const
		{
			return m_EntityHandle != entt::null && m_Scene != nullptr;
		}

		/**
		 * @brief 
		 */
		operator entt::entity() const
		{
			return m_EntityHandle;
		}

		/**
		 * @brief 
		 */
		operator uint32_t() const
		{
			return (uint32_t)m_EntityHandle;
		}

		/**
		 * @brief 
		 * @param other 
		 * @return 
		 */
		bool operator==(const Entity& other) const 
		{ 
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene; 
		}

		/**
		 * @brief 
		 * @param other 
		 * @return 
		 */
		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

		// ---- Hierarchy (Replaces Node) ----

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] Entity GetParent() const
		{
			const UUID parentId = GetComponent<RelationshipComponent>().pParentHandle;
			if (static_cast<uint64_t>(parentId) == 0)
				return {};

			return Scene::TryGetEntityWithUUID(parentId);
		}

		/**
		 * @brief 
		 * @param parent 
		 */
		void SetParent(Entity parent)
		{
			auto& relationship = GetComponent<RelationshipComponent>();
			relationship.pParentHandle = parent.GetUUID();
			parent.GetComponent<RelationshipComponent>().pChildren.push_back(GetUUID());
			Scene::MarkStructureChanged();
		}

	// -------------------------------------------------------

		/**
		 * @brief Constructs and stores a struct component of type T in place.
		 * @tparam T A non-Component struct type.
		 * @param args Constructor arguments forwarded to T.
		 * @return Reference to the newly added component.
		 */
		template<typename T, typename... Args>
		T &AddComponent(Args &&...args) requires(!ComponentTypeResolver<T>::IS_REGISTERED)
		{
			entt::registry* registry = GetRegistry();
			SEDX_CORE_ASSERT(registry, "AddComponent on an entity that isn't part of a scene");
			return registry->emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		/**
		 * @brief Adds a runtime component of type T. Returns existing component if already present.
		 * @tparam T A class derived from Component.
		 * @return Pointer to the component, or nullptr when the entity isn't a live scene entity.
		 */
		template<typename T>
		T *AddComponent() requires(ComponentTypeResolver<T>::IS_REGISTERED && std::derived_from<T, Component>)
		{
			if (T* existing = GetComponent<T>())
			{
				return existing;
			}

			// The component's owner is the entity's stored handle, which outlives this (possibly temporary) one.
			Entity *stored = GetStored();
			if (!stored)
				return nullptr;

			auto component = CreateRef<T>(stored);
			T *added = component.Get();
			AttachComponent(Component::TypeToEnum<T>(), component);
			return added;
		}

		template<typename T>
		T *AddComponent() requires(ComponentTypeResolver<T>::IS_REGISTERED && !std::derived_from<T, Component>)
		{
			return nullptr;
		}

		// ---- RemoveComponent ----

		/**
		 * @brief Removes a struct component of type T from this entity.
		 * @tparam T A non-Component struct type.
		 */
		template<typename T>
		void RemoveComponent() requires(!ComponentTypeResolver<T>::IS_REGISTERED)
		{
			if (entt::registry* registry = GetRegistry())
				registry->remove<T>(m_EntityHandle);
		}

		/**
		 * @brief Removes a runtime component of type T from this entity.
		 * @tparam T A class derived from Component.
		 */
		template<typename T>
		void RemoveComponent() requires(ComponentTypeResolver<T>::IS_REGISTERED && std::derived_from<T, Component>)
		{
			RemoveComponentByType(Component::TypeToEnum<T>());
		}

		template<typename T>
		void RemoveComponent() requires(ComponentTypeResolver<T>::IS_REGISTERED && !std::derived_from<T, Component>)
		{
		}

		// ---- TryGetComponent ----

		/**
		 * @brief Returns a pointer to a struct component of type T, or nullptr if not present.
		 * @tparam T A non-Component struct type.
		 */
		template<typename T>
		T *TryGetComponent() requires(!ComponentTypeResolver<T>::IS_REGISTERED)
		{
			entt::registry* registry = GetRegistry();
			return registry ? registry->try_get<T>(m_EntityHandle) : nullptr;
		}

		/**
		 * @brief Returns a const pointer to a struct component of type T, or nullptr if not present.
		 * @tparam T A non-Component struct type.
		 */
		template<typename T>
		const T *TryGetComponent() const requires(!ComponentTypeResolver<T>::IS_REGISTERED)
		{
			const entt::registry* registry = GetRegistry();
			return registry ? registry->try_get<T>(m_EntityHandle) : nullptr;
		}

		/**
		 * @brief Returns a pointer to a runtime component of type T, or nullptr if not present.
		 * @tparam T A class derived from Component.
		 */
		template<typename T>
		T *TryGetComponent() requires(ComponentTypeResolver<T>::IS_REGISTERED && std::derived_from<T, Component>)
		{
			return GetComponent<T>();
		}

		template<typename T>
		T *TryGetComponent() requires(ComponentTypeResolver<T>::IS_REGISTERED && !std::derived_from<T, Component>)
		{
			return nullptr;
		}

		/**
		 * @brief Returns a const pointer to a runtime component of type T, or nullptr if not present.
		 * @tparam T A class derived from Component.
		 */
		template<typename T>
		const T *TryGetComponent() const requires(ComponentTypeResolver<T>::IS_REGISTERED && std::derived_from<T, Component>)
		{
			return GetComponent<T>();
		}

		template<typename T>
		const T *TryGetComponent() const requires(ComponentTypeResolver<T>::IS_REGISTERED && !std::derived_from<T, Component>)
		{
			return nullptr;
		}

		// ---- HasComponent helpers ----

		/**
		 * @brief Returns true if the entity has a struct component of type T.
		 */
		template<typename T>
		bool HasComponent() requires(!ComponentTypeResolver<T>::IS_REGISTERED)
		{
			return HasSingleComponent<T>();
		}

		/**
		 * @brief Returns true if the entity has a runtime Component of type T.
		 */
		template<typename T>
		bool HasComponent() requires(ComponentTypeResolver<T>::IS_REGISTERED && std::derived_from<T, Component>)
		{
			return GetComponentByType(Component::TypeToEnum<T>()) != nullptr;
		}

		template<typename T>
		bool HasComponent() requires(ComponentTypeResolver<T>::IS_REGISTERED && !std::derived_from<T, Component>)
		{
			return false;
		}

		// ---- HasComponent / HasAny ----

		template<typename T>
		bool HasComponent()
		{
			return HasSingleComponent<T>();
		}

		template<typename T>
		[[nodiscard]] bool HasComponent() const
		{
			return HasSingleComponent<T>();
		}

		/**
		 * @brief Returns true if the entity has ALL of the specified component types.
		 * @tparam T... One or more component types to check.
		 */
		template<typename... T>
		requires (sizeof...(T) > 1)
		bool HasComponent()
		{
			return (HasSingleComponent<T>() && ...);
		}

		/* @brief Returns true if the entity has ALL the specified component types (const version). */
		template<typename... T>
		requires (sizeof...(T) > 1)
		[[nodiscard]] bool HasComponent() const
		{
			return (HasSingleComponent<T>() && ...);
		}

		/**
		 * @brief Returns true if the entity has ANY of the specified component types.
		 * @tparam T... One or more component types to check.
		 */
		template<typename... T>
		bool HasAny()
		{
			return (HasSingleComponent<T>() || ...);
		}

		/**
		 * @brief Returns true if the entity has ANY of the specified component types (const version).
		 * @tparam T 
		 * @return 
		 */
		template<typename... T>
		[[nodiscard]] bool HasAny() const
		{
			return (HasSingleComponent<T>() || ...);
		}

		/**
		 * @brief Removes component of type T if present; no-op if absent.
		 * @tparam T The component type to remove.
		 */
		template<typename T>
		void RemoveComponentIfExists()
		{
			if (HasSingleComponent<T>())
			{
				RemoveComponent<T>();
			}
		}

		/**
		 * @brief The entity's name (its TagComponent).
		 * @return The tag, or "Unnamed" when the entity has none.
		 */
		[[nodiscard]] const std::string& Name() const
		{
			static const std::string s_Unnamed = "Unnamed";
			const TagComponent* tag = TryGetComponent<TagComponent>();
			return tag ? tag->pTag : s_Unnamed;
		}

		//operator uint32_t () const { return (uint32_t)m_EntityHandle; }
		//operator entt::entity () const { return m_EntityHandle; }
		//operator bool () const;

		/*
		bool operator==(const Entity& other) const
		{
			return GetUUID() == other.GetUUID() && m_Scene == other.m_Scene;
		}
		*/

		/*
		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}
		*/

		/*
		[[nodiscard]] Entity GetParent() const;
		*/

		/*
		void SetParent(Entity parent)
		{
			Entity currentParent = GetParent();
			if (currentParent == parent)
			{
				return;
			}

			// If changing parent, remove child from existing parent
			if (currentParent)
			{
				currentParent.RemoveChild(*this);
			}

			// Setting to null is okay
			SetParentUUID(parent.GetUUID());

			if (parent)
			{
				auto& parentChildren = parent.Children();
				if (UUID uuid = GetUUID(); std::ranges::find(parentChildren, uuid) == parentChildren.end())
				{
					parentChildren.emplace_back(GetUUID());
				}
			}
		}
		*/

		/**
		 * @brief 
		 * @param parent 
		 */
		void SetParentUUID(UUID parent)
		{
			GetComponent<RelationshipComponent>().pParentHandle = std::move(parent);
			Scene::MarkStructureChanged();
		}

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] UUID GetParentUUID() const { return GetComponent<RelationshipComponent>().pParentHandle; }

		/**
		 * @brief 
		 * @return 
		 */
		std::vector<UUID>& Children() { return GetComponent<RelationshipComponent>().pChildren; }

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] const std::vector<UUID>& Children() const { return GetComponent<RelationshipComponent>().pChildren; }

		/**
		 * @brief 
		 * @param child 
		 * @return 
		 */
		bool RemoveChild(Entity child)
		{
			UUID childId = child.GetUUID();
			std::vector<UUID>& children = Children();
			if (auto it = std::ranges::find(children, childId); it != children.end())
			{
				children.erase(it);
				return true;
			}

			return false;
		}

		/**
		 * @brief 
		 * @param entity 
		 * @return 
		 */
		[[nodiscard]] bool IsAncestorOf(Entity entity) const;

		/**
		 * @brief 
		 * @param entity 
		 * @return 
		 */
		[[nodiscard]] bool IsDescendantOf(const Entity &entity) const { return entity.IsAncestorOf(*this); }

		/**
		 * @brief 
		 * @return 
		 */
		TransformComponent& Transform() { return GetComponent<TransformComponent>(); }

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] const Mat4& Transform() const
		{
			return GetComponent<TransformComponent>().GetTransform();
		}

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] UUID GetUUID() const
		{
			return GetComponent<IDComponent>().pID;
		}

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] UUID GetSceneUUID() const;

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] float GetTimeSinceLastTransform() const;

	private:
		entt::entity m_EntityHandle{entt::null}; // Handle used for registry lookups; entt::null if invalid
		Scene *m_Scene = nullptr; // Non-owning pointer to the scene this entity belongs to; nullptr if invalid
		EntityRuntime *m_Runtime = nullptr; // The entity's runtime record when this handle came from the scene (see GetRuntime)

		/**
		 * @brief The registry holding this entity's struct components (ID, tag, transform, ...).
		 * @return The scene registry, or nullptr when the entity is not bound to a live scene entity.
		 */
		[[nodiscard]] entt::registry* GetRegistry() const
		{
			if (!m_Scene || m_EntityHandle == entt::null || !m_Scene->m_Registry.valid(m_EntityHandle))
				return nullptr;

			return &m_Scene->m_Registry;
		}

		/**
		 * @brief The entity's runtime record: the one this handle carries unless that entity was destroyed (the scene
		 * clears a destroyed record's handle and recycles the record), else the scene's lookup by handle.
		 * @return The record, or nullptr when the handle isn't a live entity of the active scene.
		 */
		[[nodiscard]] EntityRuntime* GetRuntime() const;

		/* @brief The stored handle in the runtime record (owner of runtime components), or nullptr. */
		[[nodiscard]] Entity* GetStored() const;

		/* @brief Stores a new runtime component in the record, then sets its type, calls Init() and marks the scene structure changed. */
		void AttachComponent(ComponentType type, const Ref<Component> &component);

		/**
		 * @brief Returns true if this entity has a single component of type T.
		 * @tparam T A struct component or a Component subclass.
		 */
		template<typename T>
		[[nodiscard]] bool HasSingleComponent() const
		{
			if constexpr (ComponentTypeResolver<T>::IS_REGISTERED)
			{
				return GetComponentByType(Component::TypeToEnum<T>()) != nullptr;
			}
			else
			{
				const entt::registry* registry = GetRegistry();
				return registry && registry->all_of<T>(m_EntityHandle);
			}
		}

		friend struct EntityRuntime;
		friend class Prefab;
		friend class Scene;
		friend class SceneSerializer;
		friend class ScriptEngine;
	};

	/**
	 * @struct EntityRuntime
	 * @brief The scene-owned, per-entity state that doesn't live in the registry: runtime components, the active
	 * flag and the previous world matrix. One per live entity at a fixed address (Scene pools them); `entity` is the
	 * entity's stored handle, the one Scene::GetStoredEntity() / GetEntities() and Component::GetEntity() return.
	 */
	struct EntityRuntime
	{
		Entity entity;

		// Runtime components (Component subclasses), indexed by ComponentType enum
		std::array<Ref<Component>, static_cast<uint32_t>(ComponentType::MaxEnum)> components{};

		// world transform the renderer used last frame (motion vectors)
		xMath::Matrix matrixPrevious = xMath::Matrix::IDENTITY;
		bool hasMatrixPrevious = false;

		bool active = true; // SetActive(); read by the renderer when collecting draw calls
		float timeSinceLastTransform = 0.0f; // Used for editor gizmo display and other transform-related time tracking (time in seconds)
	};

	static_assert(std::is_trivially_copyable_v<Entity>, "Entity is a handle: copying it must stay a plain copy");

	inline EntityRuntime *Entity::GetRuntime() const
	{
		if (m_Runtime && m_Runtime->entity.m_EntityHandle == m_EntityHandle && m_EntityHandle != entt::null)
			return m_Runtime;

		return Scene::FindRuntime(m_EntityHandle, m_Scene);
	}

	inline Entity *Entity::GetStored() const
	{
		EntityRuntime *runtime = GetRuntime();
		return runtime ? &runtime->entity : nullptr;
	}

	inline Component *Entity::GetComponentByType(const ComponentType type) const
	{
		const uint32_t index = static_cast<uint32_t>(type);
		const EntityRuntime *runtime = GetRuntime();
		if (!runtime || index >= runtime->components.size())
			return nullptr;

		return runtime->components[index].Get();
	}

	inline bool Entity::GetActive() const
	{
		const EntityRuntime *runtime = GetRuntime();
		return runtime && runtime->active;
	}

	inline void Entity::SetActive(const bool active)
	{
		if (EntityRuntime *runtime = GetRuntime())
			runtime->active = active;
	}

	inline void Entity::SetMatrixPrevious(const xMath::Matrix &m)
	{
		if (EntityRuntime *runtime = GetRuntime())
		{
			runtime->matrixPrevious    = m;
			runtime->hasMatrixPrevious = true;
		}
	}

	inline xMath::Matrix Entity::GetMatrixPrevious() const
	{
		const EntityRuntime *runtime = GetRuntime();
		return runtime && runtime->hasMatrixPrevious ? runtime->matrixPrevious : GetMatrix();
	}

	inline float Entity::GetTimeSinceLastTransform() const
	{
		const EntityRuntime *runtime = GetRuntime();
		return runtime ? runtime->timeSinceLastTransform : 0.0f;
	}

}


// -------------------------------------------------------
