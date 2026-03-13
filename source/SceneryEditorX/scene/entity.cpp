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

// -------------------------------------------------------

namespace SceneryEditorX
{
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
		return false;
	}

	Entity::operator bool() const { return IsValid(); }

}

// -------------------------------------------------------
