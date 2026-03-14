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
 * node.h
 * -------------------------------------------------------
 * Created: 16/4/2025
 * -------------------------------------------------------
 */
// ReSharper disable CppInconsistentNaming
#pragma once
#include <SceneryEditorX/core/identifiers/uuid.h>
#include <SceneryEditorX/utils/inheritance.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{

	class Node : public SharedObject
	{
	public:
		Node() = default;
		virtual ~Node() override = default;
		//virtual void Serialize(Serializer &ser);

		/**
		 * @brief Recursively searches through this node and all its descendants to find all nodes of type T and adds them to the provided vector.
		 * @tparam T The type of nodes to search for. This should be a class that inherits from Node.
		 * @param all A vector to store the found nodes of type T.
		 */
		template <typename T>
		void GetAll(std::vector<Ref<T>> &all)
		{
			for (auto &node : m_Children)
			{
				if (auto typed = std::dynamic_pointer_cast<T>(node))
					all.emplace_back(typed);

				node->GetAll(all);
			}
		}

		/**
		 * @brief Recursively searches through this node and all its descendants to find all nodes of type T and returns them in a vector.
		 * @tparam T The type of nodes to search for. This should be a class that inherits from Node.
		 * @return A vector of Ref<T> containing all nodes of type T found in this node and its descendants. 
		 * @note If no nodes of type T are found, the vector will be empty.
		 */
		template <typename T>
		std::vector<Ref<T>> GetAll()
		{
			std::vector<Ref<T>> all;
			for (auto &node : m_Children)
			{
				if (auto typed = std::dynamic_pointer_cast<T>(node))
					all.emplace_back(typed);

				node->GetAll(all);
			}
			return all;
		}

		/**
		 * @brief Sets the parent of a child node.
		 * If the child already has a parent, it will be removed from the old parent's children list before being added to the new parent's children list.
		 * This ensures that the child is not left in an inconsistent state with multiple parents or orphaned references.
		 *
		 * @param child The child node whose parent is being set
		 * @param parent The new parent node
		 */
		static void SetParent(const Ref<Node> &child, const Ref<Node> &parent)
		{
			if (child->m_Parent)
			{
				const Ref<Node> oldParent = child->m_Parent;
				const auto it = std::ranges::find_if(oldParent->m_Children, [&](auto &n)
				{
					return child->m_ID == n->m_ID;
				});
				SEDX_ASSERT(it != oldParent->m_Children.end(), "Child not found in children vector");
				oldParent->m_Children.erase(it);
			}
			child->m_Parent = parent;
			parent->m_Children.push_back(child);
		}

		/**
		 * @brief Recursively updates the parent reference of all children nodes to point to the given node.
		 * @param node The node whose children will have their parent reference updated to point to this node
		 * @note This should be called after changing a node's parent to ensure all descendants have correct parent references.
		 */
		static void UpdateChildrenParent(const Ref<Node> &node)
		{
			for (auto &child : node->m_Children)
			{
				child->m_Parent = node;
				UpdateChildrenParent(child);
			}
		}

		/**
		 * @brief Creates a deep copy of the given node and all its children
		 * @param node The node to clone
		 * @return A new Ref<Node> that is a deep copy of the input node, including all its children
		 */
		static Ref<Node> Clone(Ref<Node> &node);
	
		Mat4 GetLocalTransform() const;
		Mat4 GetWorldTransform() const;
		Vec3 GetWorldPosition() const;
		Mat4 GetParentTransform() const;
		Vec3 GetWorldFront() const;

        /**
		 * @brief Composes a transformation matrix from position, rotation, and scale vectors, optionally applying a parent transformation.
		 * @param pos The position vector
		 * @param rot The rotation vector
		 * @param scl The scale vector
		 * @param parent The parent transformation matrix
		 * @return The composed transformation matrix
		 */
		static Mat4 ComposeTransform(const Vec3 &pos, const Vec3 &rot, const Vec3 &scl, const Mat4 &parent = Mat4(1));

	protected:
		std::vector<Ref<Node>> m_Children;
		std::string m_State;

		Vec3 position = Vec3(0.0f);
		Vec3 rotation = Vec3(0.0f);
		Vec3 scale = Vec3(1.0f);

	private:
		Ref<Node> m_Parent;
		UUID m_ID;
		friend class AssetManager;
	};

}

// ------------------------------------------------
