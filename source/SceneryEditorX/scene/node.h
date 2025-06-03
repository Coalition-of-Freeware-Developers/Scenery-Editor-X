/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* node.h
* -------------------------------------------------------
* Created: 16/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/scene/asset.h>

/// ---------------------------------------------------------

namespace SceneryEditorX
{
	/*
	struct Serializer;
	class SceneAsset;

    /// -------------------------------------------------------

	class Node : public Object
	{
	public:

        Node();
        virtual void Serialize(Serializer &ser) override;

		/// -------------------------------------------------------

        Ref<Node> parent;
	    std::vector<Ref<Node>> children;
	    Vec3 position = Vec3(0.0f);
	    Vec3 rotation = Vec3(0.0f);
	    Vec3 scale = Vec3(1.0f);

		/// -------------------------------------------------------
	
	    template <typename T>
	    void GetAll(ObjectType type, std::vector<Ref<T>> &all)
	    {
	        for (auto &node : children)
	        {
	            if (node->type == type)
	            {
	                all.emplace_back(std::dynamic_pointer_cast<T>(node));
	            }
	            node->GetAll(type, all);
	        }
	    }
	
	    template <typename T>
	    std::vector<Ref<T>> GetAll(ObjectType type)
	    {
	        std::vector<Ref<T>> all;
	        for (auto &node : children)
	        {
	            if (node->type == type)
	            {
	                all.emplace_back(std::dynamic_pointer_cast<T>(node));
	            }
	            node->GetAll(type, all);
	        }
	        return all;
	    }
	
	    static void SetParent(const Ref<Node> &child, const Ref<Node> &parent)
	    {
	        if (child->parent)
	        {
	            const Ref<Node> oldParent = child->parent;
	            const auto it = std::ranges::find_if(oldParent->children, [&](auto &n) {
	                return child->uuid == n->uuid;
	            });
	            SEDX_ASSERT(it != oldParent->children.end(), "Child not found in children vector");
	            oldParent->children.erase(it);
	        }
	        child->parent = parent;
	        parent->children.push_back(child);
	    }
	
	    static void UpdateChildrenParent(const Ref<Node> &node)
	    {
	        for (auto &child : node->children)
	        {
	            child->parent = node;
	            UpdateChildrenParent(child);
	        }
	    }
	
	    static Ref<Node> Clone(Ref<Node> &node);
	
	    glm::mat4 GetLocalTransform();
	    glm::mat4 GetWorldTransform();
	    Vec3 GetWorldPosition();
	    glm::mat4 GetParentTransform();
	    Vec3 GetWorldFront();
	    static glm::mat4 ComposeTransform(const Vec3 &pos,
	                                      const Vec3 &rot,
	                                      const Vec3 &scl,
										  const glm::mat4 &parent = glm::mat4(1));

	private:

		friend class AssetManager;
	};
	*/

} // namespace SceneryEditorX

// ------------------------------------------------
