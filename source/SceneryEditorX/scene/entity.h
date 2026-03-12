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
//#include <entt/src/entt/entt.hpp>
#include "components.h"
#include <SceneryEditorX/utils/inheritance.h>

// -------------------------------------------------------


namespace SceneryEditorX
{
	class Scene;

	class Entity : public SharedObject
	{
	public:
		Entity();
		//Entity(/*entt::entity handle,*/ Scene* scene) : /*m_EntityHandle(handle),*/ m_Scene(scene) {}
		~Entity();

		void Start();
		void Stop();
		void Tick();

		bool GetActive();
		void SetActive(const bool active);

		[[nodiscard]] bool IsValid() const;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args);

		template<typename T>
		T& GetComponent();

		template<typename T>
		const T& GetComponent() const;
		
		Component* GetComponentByType(ComponentType Type) const;
		Component* AddComponentByType(ComponentType Type);
		void RemoveComponentByType(ComponentType Type);

		// adds a component of type T
		template <class T>
		T* AddComponent()
		{
			const ComponentType type = Component::TypeToEnum<T>();

			// early exit if the component exists
			if (T* component = GetComponent<T>())
				return component;

			// create a new component
			std::shared_ptr<T> component = std::make_shared<T>(this);

			// save new component
			m_components[static_cast<uint32_t>(type)] = std::static_pointer_cast<Component>(component);

			// initialize component
			component->SetType(type);
			component->Initialize();

			return component.get();
		}

		// adds a component of ComponentType
		Component* AddComponent(ComponentType type);

		// returns a component of type T
		template <class T>
		T* GetComponent()
		{
			const ComponentType component_type = Component::TypeToEnum<T>();
			return static_cast<T*>(m_components[static_cast<uint32_t>(component_type)].get());
		}

		// removes a component
		template <class T>
		void RemoveComponent()
		{
			const ComponentType component_type = Component::TypeToEnum<T>();
			m_components[static_cast<uint32_t>(component_type)] = nullptr;
		}
		// returns nullptr if entity does not have the requested component type
		template<typename T>
		T* TryGetComponent();

		// returns nullptr if entity does not have the requested component type
		template<typename T>
		const T* TryGetComponent() const;

		template<typename... T>
		bool HasComponent();

		template<typename... T>
		[[nodiscard]] bool HasComponent() const;

		template<typename...T>
		bool HasAny();

		template<typename...T>
		[[nodiscard]] bool HasAny() const;

		template<typename T>
		void RemoveComponent();

		template<typename T>
		void RemoveComponentIfExists();

		std::string& Name()
		{
			return HasComponent<TagComponent>() ? GetComponent<TagComponent>().tag : m_NoName;
		}

		[[nodiscard]] const std::string& Name() const
		{
			return HasComponent<TagComponent>() ? GetComponent<TagComponent>().tag : m_NoName;
		}

		//operator uint32_t () const { return (uint32_t)m_EntityHandle; }
		//operator entt::entity () const { return m_EntityHandle; }
		operator bool () const;

		bool operator==(const Entity& other) const
		{
			return GetUUID() == other.GetUUID() && m_Scene == other.m_Scene;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

		[[nodiscard]] Entity GetParent() const;

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

		void SetParentUUID(UUID parent) { GetComponent<RelationshipComponent>().parentHandle = std::move(parent); }
		[[nodiscard]] UUID GetParentUUID() const { return GetComponent<RelationshipComponent>().parentHandle; }
		std::vector<UUID>& Children() { return GetComponent<RelationshipComponent>().children; }
		[[nodiscard]] const std::vector<UUID>& Children() const { return GetComponent<RelationshipComponent>().children; }

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

		[[nodiscard]] UUID GetUUID() const { return GetComponent<IDComponent>().id; }
		[[nodiscard]] UUID GetSceneUUID() const;

	private:
		Entity(const std::string& name);

		//entt::entity m_EntityHandle{ entt::null };
		Scene *m_Scene = nullptr;

		inline static std::string m_NoName = "Unnamed";

		friend class Prefab;
		friend class Scene;
		friend class SceneSerializer;
		friend class ScriptEngine;
	};

}


// -------------------------------------------------------
