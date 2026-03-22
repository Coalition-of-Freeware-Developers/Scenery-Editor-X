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
#include <type_traits>
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
		virtual ~Entity() = default;

		void Start();
		void Stop();
		void Tick();

		/**
		 * @brief 
		 * @return 
		 */
		bool GetActive();

		/**
		 * @brief 
		 * @param active 
		 */
		virtual void SetActive(const bool active);

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] bool IsValid() const;

		/**
		 * @brief 
		 * @return  
		 */
		virtual xMath::Matrix GetMatrix() const { return xMath::Matrix{}; }

		/**
		 * @brief 
		 */
		virtual void SetMatrixPrevious(const xMath::Matrix& /*m*/) {}

		/**
		 * @brief 
		 * @return 
		 */
		virtual xMath::Vec3 GetPosition() const;

		/**
		 * @brief 
		 * @param vector3 
		 * @return 
		 */
		virtual xMath::Vec3 SetPosition(xMath::Vec3 vector3);

		/**
		 * @brief Gets this entity's local-space position.
		 * @return Local translation.
		 */
		virtual xMath::Vec3 GetPositionLocal() const;

		/**
		 * @brief Sets this entity's local-space position.
		 * @param vector3 Local translation to apply.
		 * @return Applied local translation.
		 */
		virtual xMath::Vec3 SetPositionLocal(xMath::Vec3 vector3);

		/**
		 * @brief 
		 * @return 
		 */
		virtual xMath::Vec3 GetRotation() const;

		/**
		 * @brief 
		 * @param vector3 
		 * @return 
		 */
		virtual xMath::Vec3 SetRotation(xMath::Vec3 vector3);

		/**
		 * @brief Gets this entity's local-space euler rotation (radians).
		 * @return Local rotation in radians.
		 */
		virtual xMath::Vec3 GetRotationLocal() const;

		/**
		 * @brief Sets this entity's local-space euler rotation (radians).
		 * @param vector3 Local rotation in radians.
		 * @return Applied local rotation in radians.
		 */
		virtual xMath::Vec3 SetRotationLocal(xMath::Vec3 vector3);

		/**
		 * @brief 
		 * @return 
		 */
		virtual xMath::Vec3 GetScale() const;

		/**
		 * @brief 
		 * @param vector3 
		 * @return 
		 */
		virtual xMath::Vec3 SetScale(xMath::Vec3 vector3);

		/**
		 * @brief Gets this entity's local-space scale.
		 * @return Local scale.
		 */
		virtual xMath::Vec3 GetScaleLocal() const;

		/**
		 * @brief Sets this entity's local-space scale.
		 * @param vector3 Local scale to apply.
		 * @return Applied local scale.
		 */
		virtual xMath::Vec3 SetScaleLocal(xMath::Vec3 vector3);

		/**
		 * @brief Gets this entity's local-space pivot point.
		 * @return Local pivot point.
		 */
		virtual xMath::Vec3 GetPivotPoint() const;

		/**
		 * @brief Sets this entity's local-space pivot point.
		 * @param vector3 Local pivot point to apply.
		 */
		virtual void SetPivotPoint(const Vec3& vector3);

		/**
		 * @brief 
		 * @return 
		 */
		virtual Vec3 GetForward() const;

		/**
		 * @brief 
		 * @return 
		 */
		virtual Vec3 GetUp() const;

		/**
		 * @brief 
		 * @return 
		 */
		virtual Vec3 GetDown() const;

		/**
		 * @brief 
		 * @return 
		 */
		virtual Vec3 GetLeft() const;

		/**
		 * @brief 
		 * @return 
		 */
		virtual Vec3 GetRight() const;

		/**
		 * @brief 
		 * @param Type 
		 * @return 
		 */
		Component* GetComponentByType(ComponentType Type) const;

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
		const T &GetComponent() const requires(!ComponentTypeResolver<T>::IS_REGISTERED)
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
		T *GetComponent() requires(ComponentTypeResolver<T>::IS_REGISTERED && std::derived_from<T, Component>)
		{
			const ComponentType type = Component::TypeToEnum<T>();
			return static_cast<T*>(m_components[static_cast<uint32_t>(type)].Get());
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
			const ComponentType type = Component::TypeToEnum<T>();
			return static_cast<const T*>(m_components[static_cast<uint32_t>(type)].Get());
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

		/**
		 * @brief 
		 * @return 
		 */
		UUID GetUUID()
		{
			return GetComponent<IDComponent>().pID;
		}

		/**
		 * @brief 
		 * @return 
		 */
		const std::string& Name()
		{
			return GetComponent<TagComponent>().pTag;
		}

		// ---- Hierarchy (Replaces Node) ----

		/**
		 * @brief 
		 * @return 
		 */
		Entity GetParent() const
		{
			UUID parentId = GetComponent<RelationshipComponent>().pParentHandle;
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
			m_structComponents[std::type_index(typeid(T))] = T(std::forward<Args>(args)...);
			return std::any_cast<T&>(m_structComponents[std::type_index(typeid(T))]);
		}

		/**
		 * @brief Adds a runtime component of type T. Returns existing component if already present.
		 * @tparam T A class derived from Component.
		 * @return Pointer to the component.
		 */
		template<typename T>
		T *AddComponent() requires(ComponentTypeResolver<T>::IS_REGISTERED && std::derived_from<T, Component>)
		{
			const ComponentType type = Component::TypeToEnum<T>();
			if (T* existing = GetComponent<T>())
			{
				return existing;
			}

			auto component = CreateRef<T>(this);
			m_components[static_cast<uint32_t>(type)] = component;
			component->SetType(type);
			component->Init();
			return component.Get();
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
			m_structComponents.erase(std::type_index(typeid(T)));
		}

		/**
		 * @brief Removes a runtime component of type T from this entity.
		 * @tparam T A class derived from Component.
		 */
		template<typename T>
		void RemoveComponent() requires(ComponentTypeResolver<T>::IS_REGISTERED && std::derived_from<T, Component>)
		{
			const ComponentType type = Component::TypeToEnum<T>();
			m_components[static_cast<uint32_t>(type)] = nullptr;
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
		const T *TryGetComponent() const requires(!ComponentTypeResolver<T>::IS_REGISTERED)
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

		/*
		std::string& Name()
		{
			return HasComponent<TagComponent>() ? GetComponent<TagComponent>().pTag : m_Name;
		}
		*/

		/**
		 * @brief 
		 * @return 
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

		/**
		 * @brief 
		 * @param parent 
		 */
		void SetParentUUID(UUID parent) { GetComponent<RelationshipComponent>().pParentHandle = std::move(parent); }

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
		float GetTimeSinceLastTransform() const { return m_TimeSinceLastTransform; }

	private:
		entt::entity m_EntityHandle{entt::null}; // Handle used for registry lookups; entt::null if invalid
		Scene *m_Scene = nullptr; // Non-owning pointer to the scene this entity belongs to; nullptr if invalid
		float m_TimeSinceLastTransform = 0.0f; // Used for editor gizmo display and other transform-related time tracking (time in seconds)
		std::string m_Name = "Unnamed";

		// Struct/data components (non-Component subclasses from component_sets.h), keyed by type
		std::unordered_map<std::type_index, std::any> m_structComponents;

		// Runtime components (Component subclasses), indexed by ComponentType enum
		std::array<Ref<Component>, static_cast<uint32_t>(ComponentType::MaxEnum)> m_components{};

		/**
		 * @brief Returns true if this entity has a single component of type T.
		 * @tparam T A struct component or a Component subclass.
		 */
		template<typename T>
		[[nodiscard]] bool HasSingleComponent() const
		{
			if constexpr (ComponentTypeResolver<T>::IS_REGISTERED)
			{
				return m_components[static_cast<uint32_t>(Component::TypeToEnum<T>())] != nullptr;
			}
			else
			{
				return m_structComponents.contains(std::type_index(typeid(T)));
			}
		}

		friend class Prefab;
		friend class Scene;
		friend class SceneSerializer;
		friend class ScriptEngine;
	};

}


// -------------------------------------------------------
