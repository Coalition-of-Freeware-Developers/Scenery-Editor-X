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
 * entity.cpp
 * -------------------------------------------------------
 * Created: 11/8/2025
 * -------------------------------------------------------
 */
#include "entity.h"
#include "scene.h"
#include "components/component_sets.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		xMath::Vec3 DivideSafe(const xMath::Vec3& lhs, const xMath::Vec3& rhs)
		{
			xMath::Vec3 out = lhs;
			out.x = rhs.x != 0.0f ? lhs.x / rhs.x : lhs.x;
			out.y = rhs.y != 0.0f ? lhs.y / rhs.y : lhs.y;
			out.z = rhs.z != 0.0f ? lhs.z / rhs.z : lhs.z;
			return out;
		}

		xMath::Vec3 Multiply(const xMath::Vec3& lhs, const xMath::Vec3& rhs)
		{
			return xMath::Vec3(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
		}
	}

	void Entity::Start()
	{
	}

	void Entity::Stop()
	{
	}

	void Entity::Tick()
	{
	}

	bool Entity::GetActive()
	{
		return true;
	}

	void Entity::SetActive(const bool active)
	{
		(void)active;
	}

	xMath::Vec3 Entity::GetPosition() const
	{
		const xMath::Vec3 localPosition = GetPositionLocal();
		const Entity parent = GetParent();
		if (!parent)
		{
			return localPosition;
		}

		return parent.GetPosition() + localPosition;
	}

	xMath::Vec3 Entity::SetPosition(xMath::Vec3 vector3)
	{
		const Entity parent = GetParent();
		if (!parent)
		{
			return SetPositionLocal(vector3);
		}

		return SetPositionLocal(vector3 - parent.GetPosition());
	}

	xMath::Vec3 Entity::GetRotation() const
	{
		const xMath::Vec3 localRotation = GetRotationLocal();
		const Entity parent = GetParent();
		if (!parent)
		{
			return localRotation;
		}

		return parent.GetRotation() + localRotation;
	}

	xMath::Vec3 Entity::SetRotation(xMath::Vec3 vector3)
	{
		const Entity parent = GetParent();
		if (!parent)
		{
			return SetRotationLocal(vector3);
		}

		return SetRotationLocal(vector3 - parent.GetRotation());
	}

	xMath::Vec3 Entity::GetScale() const
	{
		const xMath::Vec3 localScale = GetScaleLocal();
		const Entity parent = GetParent();
		if (!parent)
		{
			return localScale;
		}

		return Multiply(parent.GetScale(), localScale);
	}

	xMath::Vec3 Entity::SetScale(xMath::Vec3 vector3)
	{
		const Entity parent = GetParent();
		if (!parent)
		{
			return SetScaleLocal(vector3);
		}

		return SetScaleLocal(DivideSafe(vector3, parent.GetScale()));
	}

	xMath::Vec3 Entity::GetPositionLocal() const
	{
		if (!HasComponent<TransformComponent>())
		{
			return xMath::Vec3(0.0f, 0.0f, 0.0f);
		}

		return GetComponent<TransformComponent>().translation;
	}

	xMath::Vec3 Entity::SetPositionLocal(xMath::Vec3 vector3)
	{
		if (!HasComponent<TransformComponent>())
		{
			return vector3;
		}

		auto& transform = GetComponent<TransformComponent>();
		transform.translation = vector3;
		return transform.translation;
	}

	xMath::Vec3 Entity::GetRotationLocal() const
	{
		if (!HasComponent<TransformComponent>())
		{
			return xMath::Vec3(0.0f, 0.0f, 0.0f);
		}

		return GetComponent<TransformComponent>().GetRotationEuler();
	}

	xMath::Vec3 Entity::SetRotationLocal(xMath::Vec3 vector3)
	{
		if (!HasComponent<TransformComponent>())
		{
			return vector3;
		}

		auto& transform = GetComponent<TransformComponent>();
		transform.SetRotationEuler(vector3);
		return transform.GetRotationEuler();
	}

	xMath::Vec3 Entity::GetScaleLocal() const
	{
		if (!HasComponent<TransformComponent>())
		{
			return xMath::Vec3(1.0f, 1.0f, 1.0f);
		}

		return GetComponent<TransformComponent>().scale;
	}

	xMath::Vec3 Entity::SetScaleLocal(xMath::Vec3 vector3)
	{
		if (!HasComponent<TransformComponent>())
		{
			return vector3;
		}

		auto& transform = GetComponent<TransformComponent>();
		transform.scale = vector3;
		return transform.scale;
	}

	xMath::Vec3 Entity::GetPivotPoint() const
	{
		if (!HasComponent<TransformComponent>())
		{
			return xMath::Vec3(0.0f, 0.0f, 0.0f);
		}

		return GetComponent<TransformComponent>().pivot;
	}

	void Entity::SetPivotPoint(const xMath::Vec3& vector3)
	{
		if (!HasComponent<TransformComponent>())
		{
			return;
		}

		auto& transform = GetComponent<TransformComponent>();
		transform.pivot = vector3;
	}

	Vec3 Entity::GetForward() const
	{
		if (!HasComponent<TransformComponent>())
		{
			return {0.0f, 0.0f, 1.0f};
		}

		const auto& transform = GetComponent<TransformComponent>();
		return xMath::Normalize(transform.GetRotation() * xMath::Vec3(0.0f, 0.0f, 1.0f));
	}

	Vec3 Entity::GetUp() const
	{
		if (!HasComponent<TransformComponent>())
		{
			return {0.0f, 1.0f, 0.0f};
		}

		const auto& transform = GetComponent<TransformComponent>();
		return xMath::Normalize(transform.GetRotation() * xMath::Vec3(0.0f, 1.0f, 0.0f));
	}

	Vec3 Entity::GetDown() const
	{
		if (!HasComponent<TransformComponent>())
		{
			return {0.0f, -1.0f, 0.0f};
		}

		const auto& transform = GetComponent<TransformComponent>();
		return xMath::Normalize(transform.GetRotation() * xMath::Vec3(0.0f, -1.0f, 0.0f));
	}

	Vec3 Entity::GetLeft() const
	{
		if (!HasComponent<TransformComponent>())
		{
			return {-1.0f, 0.0f, 0.0f};
		}

		const auto& transform = GetComponent<TransformComponent>();
		return xMath::Normalize(transform.GetRotation() * xMath::Vec3(-1.0f, 0.0f, 0.0f));
	}

	Vec3 Entity::GetRight() const
	{
		if (!HasComponent<TransformComponent>())
		{
			return {1.0f, 0.0f, 0.0f};
		}

		const auto& transform = GetComponent<TransformComponent>();
		return xMath::Normalize(transform.GetRotation() * xMath::Vec3(1.0f, 0.0f, 0.0f));
	}

	Component* Entity::GetComponentByType(const ComponentType type) const
	{
		const uint32_t index = static_cast<uint32_t>(type);
		if (index >= m_components.size())
			return nullptr;

		return m_components[index].Get();
	}

	Component* Entity::AddComponentByType(const ComponentType type)
	{
		const uint32_t index = static_cast<uint32_t>(type);
		if (index >= m_components.size())
			return nullptr;

		if (!m_components[index])
		{
			auto component = CreateRef<Component>(this);
			component->SetType(type);
			component->Init();
			m_components[index] = std::move(component);
		}

		return m_components[index].Get();
	}

	void Entity::RemoveComponentByType(const ComponentType type)
	{
		const uint32_t index = static_cast<uint32_t>(type);
		if (index < m_components.size())
		{
			m_components[index] = nullptr;
		}
	}

	Component* Entity::AddComponent(const ComponentType type)
	{
		return AddComponentByType(type);
	}

	/*
	Entity Entity::GetParent() const
	{
		return m_Scene->TryGetEntityWithUUID(GetParentUUID());
	}
	*/

	bool Entity::IsAncestorOf(Entity entity) const
	{
		const auto &children = Children();
	
		if (children.empty())
		{
			return false;
		}
	
		for (const UUID &child : children)
		{
			if (child == entity.GetUUID())
			{
				return true;
			}
		}
	
		/*
		for (UUID child : children)
		{
			if (m_Scene->GetEntityWithUUID(child).IsAncestorOf(entity))
			{
				return true;
			}
		}
		*/
	
		return false;
	}

	//UUID Entity::GetSceneUUID() const { return m_Scene->m_SceneID; }

	/*
	bool Entity::IsValid() const
	{
		return (m_EntityHandle != entt::null) && m_Scene && m_Scene->m_Registry.valid(m_EntityHandle);
	}
	*/

	bool Entity::IsValid() const
	{
		return (m_EntityHandle != entt::null) && m_Scene && m_Scene->m_Registry.valid(m_EntityHandle);
	}

	UUID Entity::GetSceneUUID() const
	{
		return m_Scene ? m_Scene->m_SceneID : UUID();
	}

	//Entity::operator bool() const { return IsValid(); }

}

// -------------------------------------------------------
