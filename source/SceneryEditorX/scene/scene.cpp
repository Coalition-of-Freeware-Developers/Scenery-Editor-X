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
 * scene.cpp
 * -------------------------------------------------------
 * Created: 11/4/2025
 * -------------------------------------------------------
 */
#include "scene.h"
#include "SceneryEditorX/renderer/renderer.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	//Entity* s_Camera = nullptr;

	Scene::Scene(std::string name, bool initialize) : m_Name(std::move(name))
	{
		m_SceneID = UUID();
	}

	void Scene::Init()
	{

	}

	void Scene::Shutdown()
	{
	}

	void Scene::Tick()
	{
	}

	Camera *Scene::GetCamera()
	{
		return Renderer::GetCamera();
	}

	/*
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

	Scene::Scene(const std::string &name, bool isEditorScene, bool initialize) : m_SceneID()
	{
	}

	Scene::~Scene()
	{
	}
	*/

} // namespace SceneryEditorX

// ---------------------------------------------------------
