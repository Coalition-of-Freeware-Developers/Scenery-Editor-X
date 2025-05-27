/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scene.h
* -------------------------------------------------------
* Created: 11/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/scene/asset.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/node.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    struct Serializer;
	class EditorConfig;
	
	/// -------------------------------------------------------
	
	class SceneAsset : public Asset
	{
	public:
        SceneAsset();
        virtual void Serialize(Serializer &s) override;
	    virtual void Load(const std::string &path) override;
	    virtual void Unload() override;
        bool IsLoaded() const;
        const std::string &GetPath() const;
        const std::string &GetName() const;
	    virtual void SetName(const std::string &name) override;

        std::vector<Ref<Node>> nodes;
        Vec3 ambientLightColor = Vec3(1);
        float ambientLight = 0.01f;
        int aoSamples = 4;
        int lightSamples = 2;
        float aoMin = 0.0001f;
        float aoMax = 1.0000f;
        float exposure = 2.0f;
        //ShadowType shadowType = ShadowType::ShadowRayTraced;
        uint32_t shadowResolution = 1024;

        float camSpeed = 0.01f;
        float zoomSpeed = 1.0f;
        float rotationSpeed = 0.3f;
        bool autoOrbit = false;
        Ref<CameraNode> mainCamera;

        template <typename T>
        Ref<T> Add()
        {
            Ref<T> node = std::make_shared<T>();
            nodes.push_back(node);
            return node;
        }

        void Add(const Ref<Node> &node)
        {
            nodes.push_back(node);
        }

        void DeleteRecursive(const Ref<Node> &node);

		template<typename T>
		Ref<T> Get(uint32_t id)
	    {
		    // todo: search recursively
		    for (auto& node : nodes)
			{
		        if (node->uuid == id)
				{
		            return std::dynamic_pointer_cast<T>(node);
		        }
		    }
		    return {};
		}

		template<typename T>
		void GetAll(ObjectType type, std::vector<Ref<T>>& all)
	    {
		    for (auto& node : nodes)
			{
		        if (node->type == type)
				{
		            all.emplace_back(std::dynamic_pointer_cast<T>(node));
		        }
		        node->GetAll(type, all);
		    }
		}

		template<typename T>
		std::vector<Ref<T>> GetAll(ObjectType type)
	    {
		    std::vector<Ref<T>> all;
		    for (auto& node : nodes)
			{
		        if (node->type == type)
				{
		            all.emplace_back(std::dynamic_pointer_cast<T>(node));
		        }
		        node->GetAll(type, all);
		    }
		    return all;
		}

		void UpdateParents()
	    {
		    for (auto& node : nodes)
			{
		        node->parent = {};
		        Node::UpdateChildrenParent(node);
		    }
		}

    private:
        friend class AssetManager;
        std::string scenePath;
        std::string sceneName;
        bool isLoaded = false;
	};
	
} // namespace SceneryEditorX

/// ---------------------------------------------------------
