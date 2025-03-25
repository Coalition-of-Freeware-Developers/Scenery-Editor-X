/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset_manager.cpp
* -------------------------------------------------------
* Created: 19/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/scene/asset_manager.h>
#include <SceneryEditorX/scene/serializer.h>

// -------------------------------------------------------

namespace AssetManager
{

	/*
    Object::~Object()
    {
    }

    Asset::~Asset()
    {
    }


	Mat4 Node::ComposeTransform(const Vec3 &pos, const Vec3 &rot, const Vec3 &scl, Mat4 parent)
	{
		Mat4 rotationMat = glm::toMat4(glm::quat(glm::radians(rot)));
		Mat4 translationMat = glm::translate(Mat4(1.0f), pos);
		Mat4 scaleMat = glm::scale(scl);
		return parent * (translationMat * rotationMat * scaleMat);
	}
	
	Mat4 Node::GetLocalTransform()
	{
		return ComposeTransform(position, rotation, scale);
	}
	
	Mat4 Node::GetWorldTransform()
	{
		return GetParentTransform() * GetLocalTransform();
	}
	
	Mat4 Node::GetParentTransform()
	{
		return parent ? parent->GetWorldTransform() : Mat4(1);
	}
	
	Vec3 Node::GetWorldPosition()
	{
		return GetParentTransform() * Vec4(position, 1);
	}
	
	Vec3 Node::GetWorldFront()
	{
		return GetWorldTransform() * Vec4(0, -1, 0, 0);
	}
	*/
	/*
	MeshNode::MeshNode()
	{
		type = ObjectType::MeshNode;
	}
	
	LightNode::LightNode()
	{
		type = ObjectType::LightNode;
	}
	*/

	/*
	void TextureAsset::Serialize(Serializer &s)
	{
		s.Vector("data", data);
		s("width", width);
		s("height", height);
		s("channels", channels);
	}
	
	void MeshAsset::Serialize(Serializer &s)
	{
		s.Vector("vertices", vertices);
		s.Vector("indices", indices);
	}
	
	void MaterialAsset::Serialize(Serializer &s)
	{
		s("color", color);
		s("emission", emission);
		s("metallic", metallic);
		s("roughness", roughness);
		s.Asset("colorMap", colorMap);
		s.Asset("aoMap", aoMap);
		s.Asset("emissionMap", emissionMap);
		s.Asset("normalMap", normalMap);
		s.Asset("metallicRoughnessMap", metallicRoughnessMap);
	}
	*/

	/*
	void SceneAsset::Serialize(Serializer &s)
	{
		s.VectorRef("nodes", nodes);
		s("ambientLight", ambientLight);
		s("ambientLightColor", ambientLightColor);
		s("lightSamples", lightSamples);
		s("aoSamples", aoSamples);
		s("aoMin", aoMin);
		s("aoMax", aoMax);
		s("exposure", exposure);
		//s("shadowType", shadowType);
		s("taaEnabled", taaEnabled);
		s("taaReconstruct", taaReconstruct);
		s.Node("mainCamera", mainCamera, this);
	}

	void Node::Serialize(Serializer &s)
	{
		s.VectorRef("children", children);
		s("position", position);
		s("rotation", rotation);
		s("scale", scale);
	}
	*/

	/*
	void MeshNode::Serialize(Serializer &s)
	{
		Node::Serialize(s);
		s.Asset("mesh", mesh);
		s.Asset("material", material);
	}
	*/

	/*
	void LightNode::Serialize(Serializer &s)
	{
		Node::Serialize(s);
		s("color", color);
		s("intensity", intensity);
		s("lightType", lightType);
		s("innerAngle", innerAngle);
		s("outerAngle", outerAngle);
		s("radius", radius);
		s("shadowMapRange", shadowMapRange);
		s("shadowMapFar", shadowMapFar);
		s("volumetricType", volumetricType);
	
		s("volumetricScreenAbsorption", volumetricScreenSpaceParams.absorption);
		s("volumetricScreenSamples", volumetricScreenSpaceParams.samples);
	
		s("volumetricShadowWeight", volumetricShadowMapParams.weight);
		s("volumetricShadowAbsorption", volumetricShadowMapParams.absorption);
		s("volumetricShadowDensity", volumetricShadowMapParams.density);
		s("volumetricShadowSamples", volumetricShadowMapParams.samples);
	}
	*/

    // Asset Manager
    /*
    struct AssetManagerImpl
    {
        uint32_t lastAssetsHash = 0;
        Json lastJson;
        std::filesystem::path currentProjectPath;
        std::filesystem::path currentBinPath;
        std::filesystem::path requestedProjectPath;
        std::filesystem::path requestedBinPath;
    };
	*/


	//AssetManager::AssetManager()
	//{
	//	impl = new AssetManagerImpl;
	//}
	//AssetManager::~AssetManager()
	//{
	//	delete impl;
	//}

	/*
	Ref<SceneAsset> AssetManager::GetInitialScene()
	{
		if (!initialScene)
		{
			CreateAsset<SceneAsset>("DefaultScene");
		}
		return Get<SceneAsset>(initialScene);
	}
	*/

    //std::string AssetManager::GetProjectName()
    //{
    //    return impl->currentProjectPath.stem().string();
    //}

} // namespace AssetManager
