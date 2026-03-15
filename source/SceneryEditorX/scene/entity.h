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
#include <typeindex>
#include <unordered_map>
#include <SceneryEditorX/scene/components/component.h>
#include <SceneryEditorX/utils/inheritance.h>
#include <entt/src/entt/entt.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{

	class Entity : public SharedObject
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene) : m_EntityHandle(handle), m_Scene(scene) {}
		~Entity();

		void Start();
		void Stop();
		void Tick();

		bool GetActive();
		void SetActive(const bool active);

		[[nodiscard]] bool IsValid() const;

		Component* GetComponentByType(ComponentType Type) const;
		Component* AddComponentByType(ComponentType Type);
		void RemoveComponentByType(ComponentType Type);

		/// Adds a component by ComponentType enum value
		Component* AddComponent(ComponentType type);

		// ---- Struct component access (non-Component subclasses) - returns T& ----

		/**
		 * @brief Returns a reference to a struct component of type T.
		 * @tparam T A non-Component struct type (e.g. IDComponent, TagComponent).
		 * @return Reference to the stored component.
		 */
		template<typename T>
		auto GetComponent() -> T & requires(!std::is_base_of_v<Component, T>)
		{
			auto it = m_structComponents.find(std::type_index(typeid(T)));
			SEDX_CORE_ASSERT(it != m_structComponents.end(), "Entity does not have component of this type");
			return std::any_cast<T&>(it->second);
		}

		/**
		 * @brief Returns a const reference to a struct component of type T.
		 * @tparam T A non-Component struct type.
		 * @return Const reference to the stored component.
		 */
		template<typename T>
		auto GetComponent() const -> const T& requires(!std::is_base_of_v<Component, T>)
		{
			auto it = m_structComponents.find(std::type_index(typeid(T)));
			SEDX_CORE_ASSERT(it != m_structComponents.end(), "Entity does not have component of this type");
			return std::any_cast<const T&>(it->second);
		}

		// ---- Runtime component access (Component subclasses) - returns T* ----

		/**
		 * @brief Returns a pointer to a runtime component of type T, or nullptr if absent.
		 * @tparam T A class derived from Component (e.g. CameraComponent, LightComponent).
		 * @return Pointer to the component, or nullptr.
		 */
		template<typename T>
		auto GetComponent() -> T* requires(std::is_base_of_v<Component, T>)
		{
			const ComponentType type = Component::TypeToEnum<T>();
			return static_cast<T*>(m_components[static_cast<uint32_t>(type)].get());
		}

		/**
		 * @brief Returns a const pointer to a runtime component of type T, or nullptr if absent.
		 * @tparam T A class derived from Component.
		 * @return Const pointer to the component, or nullptr.
		 */
		template<typename T>
		auto GetComponent() const -> const T* requires(std::is_base_of_v<Component, T>)
		{
			const ComponentType type = Component::TypeToEnum<T>();
			return static_cast<const T*>(m_components[static_cast<uint32_t>(type)].get());
		}

		// ---- AddComponent ----

	// ---- Component Access (Forwards to Scene Registry) ----

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			SEDX_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
			return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			SEDX_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent() const
		{
			return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			SEDX_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		// ---- Operators & Utilities ----

		operator bool() const { return m_EntityHandle != entt::null && m_Scene != nullptr; }
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }

		bool operator==(const Entity& other) const { 
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene; 
		}
		bool operator!=(const Entity& other) const { return !(*this == other); }

		UUID GetUUID() { return GetComponent<IDComponent>().pID; }
		const std::string& Name() { return GetComponent<TagComponent>().pTag; }

		// ---- Hierarchy (Replaces Node) ----
		
		Entity GetParent() const
		{
			UUID parentId = GetComponent<RelationshipComponent>().pParentHandle;
			return m_Scene->TryGetEntityWithUUID(parentId);
		}

		void SetParent(Entity parent)
		{
			auto& relationship = GetComponent<RelationshipComponent>();
			relationship.pParentHandle = parent.GetUUID();
			parent.GetComponent<RelationshipComponent>().pChildren.push_back(GetUUID());
		}

	// -------------------------------------------------------

		/**
		 * @brief Constructs and stores a struct component of type T in place.
		 * @tparam T A non-Component struct type.
		 * @param args Constructor arguments forwarded to T.
		 * @return Reference to the newly added component.
		 */
		template<typename T, typename... Args>
		auto AddComponent(Args&&... args) -> T& requires(!std::is_base_of_v<Component, T>)
		{
			m_structComponents[std::type_index(typeid(T))] = T(std::forward<Args>(args)...);
			return std::any_cast<T&>(m_structComponents[std::type_index(typeid(T))]);
		}

		/**
		 * @brief Adds a runtime component of type T. Returns existing component if already present.
		 * @tparam T A class derived from Component.
		 * @return Pointer to the component.
		 */
		template<typename T>
		auto AddComponent() -> T* requires(std::is_base_of_v<Component, T>)
		{
			const ComponentType type = Component::TypeToEnum<T>();
			if (T* existing = GetComponent<T>())
				return existing;
			auto component = std::make_shared<T>(this);
			m_components[static_cast<uint32_t>(type)] = std::static_pointer_cast<Component>(component);
			component->SetType(type);
			component->Initialize();
			return component.get();
		}

		// ---- RemoveComponent ----

		/**
		 * @brief Removes a struct component of type T from this entity.
		 * @tparam T A non-Component struct type.
		 */
		template<typename T>
		auto RemoveComponent() -> void requires(!std::is_base_of_v<Component, T>)
		{
			m_structComponents.erase(std::type_index(typeid(T)));
		}

		/**
		 * @brief Removes a runtime component of type T from this entity.
		 * @tparam T A class derived from Component.
		 */
		template<typename T>
		auto RemoveComponent() -> void requires(std::is_base_of_v<Component, T>)
		{
			const ComponentType type = Component::TypeToEnum<T>();
			m_components[static_cast<uint32_t>(type)] = nullptr;
		}

		// ---- TryGetComponent ----

		/**
		 * @brief Returns a pointer to a struct component of type T, or nullptr if not present.
		 * @tparam T A non-Component struct type.
		 */
		template<typename T>
		auto TryGetComponent() -> T* requires(!std::is_base_of_v<Component, T>)
		{
			auto it = m_structComponents.find(std::type_index(typeid(T)));
			if (it == m_structComponents.end())
				return nullptr;
			return std::any_cast<T>(&it->second);
		}

		/**
		 * @brief Returns a const pointer to a struct component of type T, or nullptr if not present.
		 * @tparam T A non-Component struct type.
		 */
		template<typename T>
		auto TryGetComponent() const -> const T* requires(!std::is_base_of_v<Component, T>)
		{
			auto it = m_structComponents.find(std::type_index(typeid(T)));
			if (it == m_structComponents.end())
				return nullptr;
			return std::any_cast<T>(&it->second);
		}

		/**
		 * @brief Returns a pointer to a runtime component of type T, or nullptr if not present.
		 * @tparam T A class derived from Component.
		 */
		template<typename T>
		auto TryGetComponent() -> T* requires(std::is_base_of_v<Component, T>)
		{
			return GetComponent<T>();
		}

		/**
		 * @brief Returns a const pointer to a runtime component of type T, or nullptr if not present.
		 * @tparam T A class derived from Component.
		 */
		template<typename T>
		auto TryGetComponent() const -> const T* requires(std::is_base_of_v<Component, T>)
		{
			return GetComponent<T>();
		}

		// ---- HasComponent / HasAny ----

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

		/**
		 * @brief Returns true if the entity has ALL the specified component types (const version).
		 */
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

		/*
		std::string& Name()
		{
			return HasComponent<TagComponent>() ? GetComponent<TagComponent>().pTag : m_Name;
		}
		*/

		[[nodiscard]] const std::string& Name() const
		{
			auto it = m_structComponents.find(std::type_index(typeid(TagComponent)));
			if (it == m_structComponents.end())
			{
				return m_Name;
			}

			return std::any_cast<const TagComponent&>(it->second).pTag;
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

		void SetParentUUID(UUID parent) { GetComponent<RelationshipComponent>().pParentHandle = std::move(parent); }
		[[nodiscard]] UUID GetParentUUID() const { return GetComponent<RelationshipComponent>().pParentHandle; }
		std::vector<UUID>& Children() { return GetComponent<RelationshipComponent>().pChildren; }
		[[nodiscard]] const std::vector<UUID>& Children() const { return GetComponent<RelationshipComponent>().pChildren; }

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

		[[nodiscard]] bool IsAncestorOf(Entity entity) const;
		[[nodiscard]] bool IsDescendantOf(Entity entity) const { return entity.IsAncestorOf(*this); }

		TransformComponent& Transform() { return GetComponent<TransformComponent>(); }
		[[nodiscard]] const Mat4& Transform() const { return GetComponent<TransformComponent>().GetTransform(); }

		[[nodiscard]] UUID GetUUID() const { return GetComponent<IDComponent>().pID; }
		[[nodiscard]] UUID GetSceneUUID() const;

	private:
		entt::entity m_EntityHandle{ entt::null };
		Scene *m_Scene = nullptr;
		std::string m_Name = "Unnamed";

		// Struct/data components (non-Component subclasses from component_sets.h), keyed by type
		std::unordered_map<std::type_index, std::any> m_structComponents;

		// Runtime components (Component subclasses), indexed by ComponentType enum
		std::array<std::shared_ptr<Component>, static_cast<uint32_t>(ComponentType::MaxEnum)> m_components{};

		/**
		 * @brief Returns true if this entity has a single component of type T.
		 * @tparam T A struct component or a Component subclass.
		 */
		template<typename T>
		[[nodiscard]] bool HasSingleComponent() const
		{
			if constexpr (std::is_base_of_v<Component, T>)
				return m_components[static_cast<uint32_t>(Component::TypeToEnum<T>())] != nullptr;
			else
				return m_structComponents.contains(std::type_index(typeid(T)));
		}

		friend class Prefab;
		friend class Scene;
		friend class SceneSerializer;
		friend class ScriptEngine;
	};

}


// -------------------------------------------------------
