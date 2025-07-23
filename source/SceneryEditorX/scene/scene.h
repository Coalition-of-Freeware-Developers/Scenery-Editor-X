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
#include <entt/src/entt/entt.hpp>
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/renderer/texture.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/node.h>
#include <SceneryEditorX/serialization/serializer.hpp>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	class Environment : public Asset
	{
	public:
		Ref<TextureCube> RadianceMap;
		Ref<TextureCube> IrradianceMap;

		Environment() = default;
		Environment(const Ref<TextureCube>& radianceMap, const Ref<TextureCube>& irradianceMap) : RadianceMap(radianceMap), IrradianceMap(irradianceMap) {}

		static ObjectType GetStaticType() { return ObjectType::EnvMap; }
		virtual ObjectType GetAssetType() const override { return GetStaticType(); }

	};

    class Scene : public Asset
    {
    public:
        explicit Scene(const std::string &name = "UntitledProject", bool isEditorScene = false, bool initialize = true);
        virtual ~Scene() override;

		virtual void Serialize(Serializer &s) override;
		virtual void Load(const std::string& path) override;
		virtual void Unload() override;
		bool IsLoaded() const { return isLoaded; }
		const std::string& GetPath() const { return scenePath; }
		const std::string& GetName() const { return m_Name; }
		virtual void SetName(const std::string& name) override;
        void Add(const Ref<CameraNode> & ref);

        static Ref<Scene> CreateEmpty();

		std::vector<Ref<GraphNode>> nodes;
		Vec3 ambientLightColor = Vec3(1);
		float ambientLight = 0.01f;
		int aoSamples = 4;
		int lightSamples = 2;
		float aoMin = 0.0001f;
		float aoMax = 1.0000f;
		float exposure = 2.0f;
		uint32_t shadowResolution = 1024;
		float camSpeed = 0.01f;
		float zoomSpeed = 1.0f;
		float rotationSpeed = 0.3f;
		bool autoOrbit = false;
        Ref<CameraNode> mainCamera;
    private:
        UUID m_SceneID;
        std::size_t m_LastSerializeHash = 0; // used by auto-save to determine if scene has changed
        entt::entity m_SceneEntity = entt::null;
        entt::registry m_Registry;

        std::function<void(AssetHandle)> m_OnSceneTransitionCallback;
        //std::function<void(Entity)> m_OnEntityDestroyedCallback;
        bool isLoaded = false;
        std::string m_Name;
        bool m_IsEditorScene = false;
        uint32_t m_ViewportTop = 0;
        uint32_t m_ViewportLeft = 0;
        uint32_t m_ViewportRight = 0;
        uint32_t m_ViewportBottom = 0;

        //EntityMap m_EntityIDMap;
        //DirectionalLight m_Light;
        float m_LightMultiplier = 0.3f;
        //LightEnvironment m_LightEnvironment;

        Ref<Environment> m_Environment;
        float m_EnvironmentIntensity = 0.0f;

        std::vector<std::function<void()>> m_PostUpdateQueue;

        float m_SkyboxLod = 1.0f;
        bool m_IsPlaying = false;
        bool m_ShouldSimulate = false;
        float m_TimeScale = 1.0f;


        friend class Entity;
        friend class Prefab;
        friend class SceneRenderer;
        friend class SceneSerializer;
        friend class PrefabSerializer;
        friend class SceneHierarchyPanel;
        friend class ECSDebugPanel;
    };

    /*
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
	*/
	
}

/// ---------------------------------------------------------
