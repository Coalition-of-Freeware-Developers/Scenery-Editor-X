/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* mesh_serializer.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#include <nlohmann/json.hpp>
#include <SceneryEditorX/logging/profiler.hpp>
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/scene/node.h>
#include <SceneryEditorX/serialization/json_serializer.h>
#include <SceneryEditorX/serialization/mesh_runtime_serializer.h>
#include <SceneryEditorX/serialization/mesh_serializer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	static std::string GetJSON(const AssetMetadata& metadata)
	{
		std::ifstream stream(Project::GetActiveAssetDirectory() / metadata.FilePath);
		SEDX_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();
		return strStream.str();
	}

	//////////////////////////////////////////////////////////////////////////////////
	// MeshSourceSerializer
	//////////////////////////////////////////////////////////////////////////////////

	std::string SerializeToJSON(Ref<Mesh> mesh)
	{
		nlohmann::json j;
		nlohmann::json meshObj;

		meshObj["MeshSource"] = mesh->GetMeshSource();

		if (auto meshSource = AssetManager::GetAsset<MeshSource>(mesh->GetMeshSource()); meshSource && meshSource->GetSubmeshes().size() == mesh->GetSubmeshes().size())
			meshObj["SubmeshIndices"] = std::vector<uint32_t>();
		else
			meshObj["SubmeshIndices"] = mesh->GetSubmeshes();

		j["Mesh"] = meshObj;
		return j.dump(4); // Pretty print with 4-space indentation
	}

	bool DeserializeFromJSON(const nlohmann::json& data, Ref<Mesh>& targetMesh)
	{
		if (!data.contains("Mesh"))
			return false;

		const auto& rootNode = data["Mesh"];
		if (!rootNode.contains("MeshAsset") && !rootNode.contains("MeshSource"))
			return false;

		AssetHandle meshSource = 0;
		if (rootNode.contains("MeshAsset")) // DEPRECATED
			meshSource = rootNode["MeshAsset"].get<uint64_t>();
		else
			meshSource = rootNode["MeshSource"].get<uint64_t>();

		// Don't return false here if the MeshSource is missing.
		// We have still loaded the asset, its just invalid.
		// This allows:
		//   - the thumbnail generator to generate a thumbnail for this mesh (it will be the "invalid" thumbnail,
		//     giving the user visual feedback that something is wrong)
		//   - asset pack builder to report missing mesh source (as opposed to just silently skipping this mesh)
		//
		//if (!AssetManager::GetAsset<MeshSource>(meshSource))
		//	return false; // TODO: feedback to the user

		auto submeshIndices = rootNode["SubmeshIndices"].get<std::vector<uint32_t>>();
		auto generateColliders = rootNode.value("GenerateColliders", false);

		targetMesh = Ref<Mesh>::Create(meshSource, submeshIndices, generateColliders);
		return true;
	}

	void RegisterMeshDependenciesFromJSON(const nlohmann::json& data, AssetHandle handle)
	{
		Project::GetEditorAssetManager()->DeregisterDependencies(handle);
		AssetHandle meshSourceHandle = 0;
		if (data.contains("Mesh"))
		{
			const auto& rootNode = data["Mesh"];
			meshSourceHandle = rootNode.value("MeshSource", static_cast<uint64_t>(0));
		}

		// must always register something, even if it's 0
		Project::GetEditorAssetManager()->RegisterDependency(meshSourceHandle, handle);
	}

	bool MeshSourceSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		SEDX_PROFILE_FUNC("MeshSourceSerializer::TryLoadData");

		AssimpMeshImporter importer(Project::GetEditorAssetManager()->GetFileSystemPathString(metadata));
		Ref<MeshSource> meshSource = importer.ImportToMeshSource();
		if (!meshSource)
			return false;

		asset = meshSource;
		asset->Handle = metadata.Handle;
		return true;
	}


	//////////////////////////////////////////////////////////////////////////////////
	// MeshSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void MeshSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<Mesh> mesh = asset.As<Mesh>();

		std::string jsonString = SerializeToJSON(mesh);

		std::filesystem::path serializePath = Project::GetActive()->GetAssetDirectory() / metadata.FilePath;
		SEDX_CORE_WARN("Serializing to {0}", serializePath.string());
		std::ofstream fout(serializePath);

		if (!fout.is_open())
		{
			SEDX_CORE_ERROR("Failed to serialize mesh to file '{0}'", serializePath);
//			SEDX_CORE_ASSERT(false, "GetLastError() = {0}", GetLastError());
			return;
		}

		fout << jsonString;
		fout.flush();
		fout.close();
	}

	bool MeshSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		Ref<Mesh> mesh;
		std::string json = GetJSON(metadata);
		nlohmann::json data = nlohmann::json::parse(json);
		bool success = DeserializeFromJSON(data, mesh);
		if (!success)
			return false;

		mesh->Handle = metadata.Handle;
		RegisterMeshDependenciesFromJSON(data, mesh->Handle);
		asset = mesh;
		return true;
	}

	void MeshSerializer::RegisterDependencies(const AssetMetadata& metadata) const
	{
		nlohmann::json data = nlohmann::json::parse(GetJSON(metadata));
		RegisterMeshDependenciesFromJSON(data, metadata.Handle);
	}

	bool MeshSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(handle);

		std::string jsonString = SerializeToJSON(mesh);
		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(jsonString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> MeshSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string jsonString;
		stream.ReadString(jsonString);

		Ref<Mesh> mesh;
		nlohmann::json data = nlohmann::json::parse(jsonString);
		bool result = DeserializeFromJSON(data, mesh);
		if (!result)
			return nullptr;

		return mesh;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// StaticMeshSerializer
	//////////////////////////////////////////////////////////////////////////////////

	std::string SerializeToJSON(Ref<StaticMesh> staticMesh)
	{
		nlohmann::json j;
		nlohmann::json meshObj;

		meshObj["MeshSource"] = staticMesh->GetMeshSource();
		meshObj["SubmeshIndices"] = staticMesh->GetSubmeshes();

		j["Mesh"] = meshObj;
		return j.dump(4); // Pretty print with 4-space indentation
	}

	bool DeserializeFromJSON(const nlohmann::json& data, Ref<StaticMesh>& targetStaticMesh)
	{
		if (!data.contains("Mesh"))
			return false;

		const auto& rootNode = data["Mesh"];
		if (!rootNode.contains("MeshAsset") && !rootNode.contains("MeshSource"))
			return false;

		AssetHandle meshSource = 0;
		if (rootNode.contains("MeshAsset")) // DEPRECATED
			meshSource = rootNode["MeshAsset"].get<uint64_t>();
		else
			meshSource = rootNode["MeshSource"].get<uint64_t>();

		// Don't return false here if the MeshSource is missing.
		// We have still loaded the asset, its just invalid.
		// This allows:
		//   - the thumbnail generator to generate a thumbnail for this mesh (it will be the "invalid" thumbnail,
		//     giving the user visual feedback that something is wrong)
		//   - asset pack builder to report missing mesh source (as opposed to just silently skipping this mesh)
		//
		//if (!AssetManager::GetAsset<MeshSource>(meshSource))
		//	return false; // TODO(Yan): feedback to the user

		auto submeshIndices = rootNode["SubmeshIndices"].get<std::vector<uint32_t>>();
		auto generateColliders = rootNode.value("GenerateColliders", true);

		targetStaticMesh = Ref<StaticMesh>::Create(meshSource, submeshIndices, generateColliders);
		return true;
	}

	void RegisterStaticMeshDependenciesFromJSON(const nlohmann::json& data, AssetHandle handle)
	{
		Project::GetEditorAssetManager()->DeregisterDependencies(handle);
		AssetHandle meshSourceHandle = 0;
		if (data.contains("Mesh"))
		{
			const auto& rootNode = data["Mesh"];
			meshSourceHandle = rootNode.value("MeshSource", static_cast<uint64_t>(0));
		}
		// must always register something, even if it's 0
		Project::GetEditorAssetManager()->RegisterDependency(meshSourceHandle, handle);
	}

	void StaticMeshSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<StaticMesh> staticMesh = asset.As<StaticMesh>();

		std::string jsonString = SerializeToJSON(staticMesh);

		auto serializePath = Project::GetActive()->GetAssetDirectory() / metadata.FilePath;
		std::ofstream fout(serializePath);
		SEDX_CORE_ASSERT(fout.good());
		if (fout.good())
			fout << jsonString;
	}

	bool StaticMeshSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		Ref<StaticMesh> staticMesh;
		std::string json = GetJSON(metadata);
		nlohmann::json data = nlohmann::json::parse(json);
		bool success = DeserializeFromJSON(data, staticMesh);
		if (!success)
			return false;

		staticMesh->Handle = metadata.Handle;
		RegisterStaticMeshDependenciesFromJSON(data, staticMesh->Handle);
		asset = staticMesh;
		return true;
	}

	void StaticMeshSerializer::RegisterDependencies(const AssetMetadata& metadata) const
	{
		nlohmann::json data = nlohmann::json::parse(GetJSON(metadata));
		RegisterStaticMeshDependenciesFromJSON(data, metadata.Handle);
	}

	bool StaticMeshSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<StaticMesh> staticMesh = AssetManager::GetAsset<StaticMesh>(handle);

		std::string jsonString = SerializeToJSON(staticMesh);
		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(jsonString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> StaticMeshSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string jsonString;
		stream.ReadString(jsonString);

		Ref<StaticMesh> staticMesh;
		nlohmann::json data = nlohmann::json::parse(jsonString);
		bool result = DeserializeFromJSON(data, staticMesh);
		if (!result)
			return nullptr;

		return staticMesh;
	}

 }

 /// -------------------------------------------------------
