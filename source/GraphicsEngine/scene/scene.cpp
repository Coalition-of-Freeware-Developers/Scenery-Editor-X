/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scene.cpp
* -------------------------------------------------------
* Created: 11/4/2025
* -------------------------------------------------------
*/
#include <GraphicsEngine/scene/scene.h>
#include <SceneryEditorX/core/serializer.hpp>
#include <algorithm> // For std::remove

/// -------------------------------------------------------

namespace SceneryEditorX
{
    SceneAsset::SceneAsset()
    {
        type = ObjectType::SceneAsset;
        sceneName = "New Scene";
    }

    void SceneAsset::Serialize(Serializer &s)
    {
        s("uuid", uuid);
        s("name", name);
        s("type", (uint8_t&)type);
        s("path", scenePath);
        s("ambientLightColor", ambientLightColor);
        s("ambientLight", ambientLight);
        s("aoSamples", aoSamples);
        s("lightSamples", lightSamples);
        s("aoMin", aoMin);
        s("aoMax", aoMax);
        s("exposure", exposure);
        //s("shadowType", (uint8_t&)shadowType);
        s("shadowResolution", shadowResolution);
        s("camSpeed", camSpeed);
        s("zoomSpeed", zoomSpeed);
        s("rotationSpeed", rotationSpeed);
        s("autoOrbit", autoOrbit);
        s.Node("mainCamera", mainCamera, this);
        s.VectorRef("nodes", nodes);
    }

    void SceneAsset::Load(const std::string &path)
    {
        scenePath = path;
        isLoaded = true;
    }

    void SceneAsset::Unload()
    {
        nodes.clear();
        mainCamera = nullptr;
        isLoaded = false;
    }

    bool SceneAsset::IsLoaded() const
    {
        return isLoaded;
    }

    const std::string &SceneAsset::GetPath() const
    {
        return scenePath;
    }

    const std::string &SceneAsset::GetName() const
    {
        return sceneName;
    }

    void SceneAsset::SetName(const std::string &name)
    {
        sceneName = name;
        this->name = name;
    }

    void SceneAsset::DeleteRecursive(const Ref<Node> &node)
    {
        /// Remove all children recursively
        for (int i = static_cast<int>(node->children.size()) - 1; i >= 0; i--)
        {
            DeleteRecursive(node->children[i]);
        }
        
        /// Remove the node from its parent's children list
        if (node->parent)
        {
            auto &children = node->parent->children;
            children.erase(std::ranges::remove(children, node).begin(), children.end());
        }
        
        /// Remove from the scene's nodes list if it's a top-level node
        if (const auto it = std::ranges::find(nodes, node); it != nodes.end())
        {
            nodes.erase(it);
        }
    }

} // namespace SceneryEditorX

/// ---------------------------------------------------------
