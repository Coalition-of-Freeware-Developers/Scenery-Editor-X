/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* mesh_runtime_serializer.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/asset/asset_manager.h>
#include <SceneryEditorX/renderer/buffers/index_buffer.h>
#include <SceneryEditorX/renderer/buffers/vertex_buffer.h>
#include <SceneryEditorX/serialization/mesh_runtime_serializer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace Utils::Animation
	{
		MemoryAllocator& GetAnimationAllocator();
	};

	struct MeshMaterial
	{
		std::string MaterialName;
		std::string ShaderName;

		glm::vec3 AlbedoColor;
		float Emission;
		float Metalness;
		float Roughness;
		bool UseNormalMap;

		uint64_t AlbedoTexture;
		uint64_t NormalTexture;
		uint64_t MetalnessTexture;
		uint64_t RoughnessTexture;

		static void Serialize(StreamWriter* serializer, const MeshMaterial& instance)
		{
			serializer->WriteString(instance.MaterialName);
			serializer->WriteString(instance.ShaderName);

			serializer->WriteRaw(instance.AlbedoColor);
			serializer->WriteRaw(instance.Emission);
			serializer->WriteRaw(instance.Metalness);
			serializer->WriteRaw(instance.Roughness);
			serializer->WriteRaw(instance.UseNormalMap);

			serializer->WriteRaw(instance.AlbedoTexture);
			serializer->WriteRaw(instance.NormalTexture);
			serializer->WriteRaw(instance.MetalnessTexture);
			serializer->WriteRaw(instance.RoughnessTexture);
		}

		static void Deserialize(StreamReader* deserializer, MeshMaterial& instance)
		{
			deserializer->ReadString(instance.MaterialName);
			deserializer->ReadString(instance.ShaderName);

			deserializer->ReadRaw(instance.AlbedoColor);
			deserializer->ReadRaw(instance.Emission);
			deserializer->ReadRaw(instance.Metalness);
			deserializer->ReadRaw(instance.Roughness);
			deserializer->ReadRaw(instance.UseNormalMap);

			deserializer->ReadRaw(instance.AlbedoTexture);
			deserializer->ReadRaw(instance.NormalTexture);
			deserializer->ReadRaw(instance.MetalnessTexture);
			deserializer->ReadRaw(instance.RoughnessTexture);
		}
	};

	static void Serialize(StreamWriter* serializer, const Skeleton& skeleton)
	{
		serializer->WriteArray(skeleton.GetBoneNames());
		serializer->WriteArray(skeleton.GetParentBoneIndices());
		serializer->WriteArray(skeleton.GetBoneTranslations());
		serializer->WriteArray(skeleton.GetBoneRotations());
		serializer->WriteArray(skeleton.GetBoneScales());
	}

	static void Deserialize(StreamReader* deserializer, Skeleton& skeleton)
	{
		std::vector<std::string> boneNames;
		std::vector<uint32_t> parentBoneIndices;
		std::vector<glm::vec3> boneTranslations;
		std::vector<glm::quat> boneRotations;
		std::vector<float> boneScales;
		deserializer->ReadArray(boneNames);
		deserializer->ReadArray(parentBoneIndices);
		deserializer->ReadArray(boneTranslations);
		deserializer->ReadArray(boneRotations);
		deserializer->ReadArray(boneScales);

		skeleton.SetBones(std::move(boneNames), std::move(parentBoneIndices), std::move(boneTranslations), std::move(boneRotations), std::move(boneScales));
	}

	static void Serialize(StreamWriter* serializer, const Animation& animation)
	{
		serializer->WriteRaw(animation.GetDuration());
		serializer->WriteRaw(animation.GetNumTracks());

		auto compressedTracks = static_cast<const acl::compressed_tracks*>(animation.GetData());
		serializer->WriteRaw(compressedTracks->get_size());
		serializer->WriteData(static_cast<const char*>(animation.GetData()), compressedTracks->get_size());
	}

	static void Deserialize(StreamReader* deserializer, Animation& animation)
	{
		uint32_t numTracks;
		float duration;
		uint32_t compressedTracksSize;

		deserializer->ReadRaw(duration);
		deserializer->ReadRaw(numTracks);

		deserializer->ReadRaw(compressedTracksSize);
		void* buffer = Utils::Animation::GetAnimationAllocator().allocate(compressedTracksSize);
		deserializer->ReadData(static_cast<char*>(buffer), compressedTracksSize);

		acl::error_result result;
		acl::compressed_tracks* compressedTracks = acl::make_compressed_tracks(buffer, &result);

		if (!compressedTracks)
		{
			SEDX_CORE_ERROR("Failed to deserialize animation: {0}", result.c_str());
			Utils::Animation::GetAnimationAllocator().deallocate(buffer, compressedTracksSize);
			return;
		}

		animation = std::move(Animation(animation.GetSkeleton(), duration, numTracks, compressedTracks));
	}

	bool MeshRuntimeSerializer::SerializeToAssetPack(uint64_t handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		outInfo.Offset = stream.GetStreamPosition();

		uint64_t streamOffset = stream.GetStreamPosition();

		Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(handle);

		MeshSourceFile file;

		bool hasMaterials = !meshSource->GetMaterials().empty();

		// The meshSource might contain some animations.  However, unless they are actually used in a scene they will not have been loaded (animation pointer is null)
		// In this case we are not interested in serializing them for runtime.
		size_t animationCount = std::count_if(std::begin(meshSource->m_Animations), std::end(meshSource->m_Animations), [](const auto& animation) { return animation.second != nullptr; });
		bool hasAnimation = animationCount != 0;

		bool hasSkeleton = meshSource->HasSkeleton();

		file.Data.Flags = 0;
		if (hasMaterials)
			file.Data.Flags |= (uint32_t)MeshSourceFile::MeshFlags::HasMaterials;
		if (hasAnimation)
			file.Data.Flags |= (uint32_t)MeshSourceFile::MeshFlags::HasAnimation;
		if (hasSkeleton)
			file.Data.Flags |= (uint32_t)MeshSourceFile::MeshFlags::HasSkeleton;

		// Write header
		stream.WriteRaw<MeshSourceFile::FileHeader>(file.Header);
		// Leave space for Metadata
		uint64_t metadataAbsolutePosition = stream.GetStreamPosition();
		stream.WriteZero(sizeof(MeshSourceFile::Metadata));

		// Write nodes
		file.Data.NodeArrayOffset = stream.GetStreamPosition() - streamOffset;
		stream.WriteArray(meshSource->m_Nodes);
		file.Data.NodeArraySize = (stream.GetStreamPosition() - streamOffset) - file.Data.NodeArrayOffset;

		// Write submeshes
		file.Data.SubmeshArrayOffset = stream.GetStreamPosition() - streamOffset;
		stream.WriteArray(meshSource->m_Submeshes);
		file.Data.SubmeshArraySize = (stream.GetStreamPosition() - streamOffset) - file.Data.SubmeshArrayOffset;

		// Write Material Buffer
		if (hasMaterials)
		{
			// Prepare materials
			std::vector<MeshMaterial> meshMaterials(meshSource->GetMaterials().size());
			const auto& meshSourceMaterials = meshSource->GetMaterials();
			for (size_t i = 0; i < meshMaterials.size(); i++)
			{
				MeshMaterial& material = meshMaterials[i];
				uint64_t meshSourceMaterialHandle = meshSourceMaterials[i];
				Ref<MaterialAsset> ma = AssetManager::GetAsset<MaterialAsset>(meshSourceMaterialHandle);
				Ref<Material> meshSourceMaterial = ma->GetMaterial();

				material.MaterialName = meshSourceMaterial->GetName();
				material.ShaderName = meshSourceMaterial->GetShader()->GetName();

				material.AlbedoColor = ma->GetAlbedoColor();
				material.Emission = ma->GetEmission();
				material.Metalness = ma->GetMetalness();
				material.Roughness = ma->GetRoughness();
				material.UseNormalMap = ma->IsUsingNormalMap();

				auto albedoTexture = ma->GetAlbedoMap();
				auto normalTexture = ma->GetNormalMap();
				auto metalnessTexture = ma->GetMetalnessMap();
				auto roughnessTexture = ma->GetRoughnessMap();

				material.AlbedoTexture = albedoTexture ? albedoTexture->Handle : uint64_t(0);
				material.NormalTexture = normalTexture ? normalTexture->Handle : uint64_t(0);
				material.MetalnessTexture = metalnessTexture ? metalnessTexture->Handle : uint64_t(0);
				material.RoughnessTexture = roughnessTexture ? roughnessTexture->Handle : uint64_t(0);
			}

			// Write materials
			file.Data.MaterialArrayOffset = stream.GetStreamPosition() - streamOffset;
			stream.WriteArray(meshMaterials);
			file.Data.MaterialArraySize = (stream.GetStreamPosition() - streamOffset) - file.Data.MaterialArrayOffset;
		}
		else
		{
			// No materials
			file.Data.MaterialArrayOffset = 0;
			file.Data.MaterialArraySize = 0;
		}

		// Write Vertex Buffer
		file.Data.VertexBufferOffset = stream.GetStreamPosition() - streamOffset;
		stream.WriteArray(meshSource->m_Vertices);
		file.Data.VertexBufferSize = (stream.GetStreamPosition() - streamOffset) - file.Data.VertexBufferOffset;

		// Write Index Buffer
		file.Data.IndexBufferOffset = stream.GetStreamPosition() - streamOffset;
		stream.WriteArray(meshSource->m_Indices);
		file.Data.IndexBufferSize = (stream.GetStreamPosition() - streamOffset) - file.Data.IndexBufferOffset;

		// Write Animation Data
		if (hasAnimation || hasSkeleton)
		{
			file.Data.AnimationDataOffset = stream.GetStreamPosition() - streamOffset;

			if (hasSkeleton)
			{
				stream.WriteArray(meshSource->m_BoneInfluences);
				stream.WriteArray(meshSource->m_BoneInfo);
				Serialize(&stream, *meshSource->m_Skeleton);
			}

			stream.WriteRaw((uint32_t)animationCount);
			for (const auto&[hash, animation] : meshSource->m_Animations)
			{
				if (animation)
				{
					stream.WriteRaw(hash);
					Serialize(&stream, *animation);
				}
			}

			file.Data.AnimationDataSize = (stream.GetStreamPosition() - streamOffset) - file.Data.AnimationDataOffset;
		}
		else
		{
			file.Data.AnimationDataOffset = 0;
			file.Data.AnimationDataSize = 0;
		}

		// Write Metadata
		uint64_t endOfStream = stream.GetStreamPosition();
		stream.SetStreamPosition(metadataAbsolutePosition);
		stream.WriteRaw<MeshSourceFile::Metadata>(file.Data);
		stream.SetStreamPosition(endOfStream);

		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> MeshRuntimeSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo)
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		uint64_t streamOffset = stream.GetStreamPosition();

		MeshSourceFile file;
		stream.ReadRaw<MeshSourceFile::FileHeader>(file.Header);
		bool validHeader = memcmp(file.Header.HEADER, "HZMS", 4) == 0;
		SEDX_CORE_ASSERT(validHeader);
		if (!validHeader)
			return nullptr;

		Ref<MeshSource> meshSource = Ref<MeshSource>::Create();
		meshSource->m_Runtime = true;

		stream.ReadRaw<MeshSourceFile::Metadata>(file.Data);

		const auto& metadata = file.Data;
		bool hasMaterials = metadata.Flags & (uint32_t)MeshSourceFile::MeshFlags::HasMaterials;
		bool hasAnimation = metadata.Flags & (uint32_t)MeshSourceFile::MeshFlags::HasAnimation;
		bool hasSkeleton = metadata.Flags & (uint32_t)MeshSourceFile::MeshFlags::HasSkeleton;

		stream.SetStreamPosition(metadata.NodeArrayOffset + streamOffset);
		stream.ReadArray(meshSource->m_Nodes);
		stream.SetStreamPosition(metadata.SubmeshArrayOffset + streamOffset);
		stream.ReadArray(meshSource->m_Submeshes);

		if (hasMaterials)
		{
			std::vector<MeshMaterial> meshMaterials;
			stream.SetStreamPosition(metadata.MaterialArrayOffset + streamOffset);
			stream.ReadArray(meshMaterials);

			meshSource->m_Materials.resize(meshMaterials.size());
			for (size_t i = 0; i < meshMaterials.size(); i++)
			{
				const auto& meshMaterial = meshMaterials[i];
				Ref<Material> material = Material::Create(Renderer::GetShaderLibrary()->Get(meshMaterial.ShaderName), meshMaterial.MaterialName);
				auto ma = Ref<MaterialAsset>::Create(material);

				ma->SetAlbedoColor(meshMaterial.AlbedoColor);
				ma->SetEmission(meshMaterial.Emission);
				ma->SetMetalness(meshMaterial.Metalness);
				ma->SetRoughness(meshMaterial.Roughness);
				ma->SetUseNormalMap(meshMaterial.UseNormalMap);

				// Get textures from AssetManager (note: this will potentially trigger additional loads)
				// TODO: set maybe to runtime error texture if no asset is present
				ma->SetAlbedoMap(meshMaterial.AlbedoTexture);
				ma->SetNormalMap(meshMaterial.NormalTexture);
				ma->SetMetalnessMap(meshMaterial.MetalnessTexture);
				ma->SetRoughnessMap(meshMaterial.RoughnessTexture);

				uint64_t maHandle = AssetManager::AddMemoryOnlyAsset(ma);
				meshSource->m_Materials[i] = maHandle;
			}
		}

		stream.SetStreamPosition(metadata.VertexBufferOffset + streamOffset);
		stream.ReadArray(meshSource->m_Vertices);

		stream.SetStreamPosition(metadata.IndexBufferOffset + streamOffset);
		stream.ReadArray(meshSource->m_Indices);

		if (hasAnimation || hasSkeleton)
		{
			stream.SetStreamPosition(metadata.AnimationDataOffset + streamOffset);
			if (hasSkeleton)
			{
				stream.ReadArray(meshSource->m_BoneInfluences);
				stream.ReadArray(meshSource->m_BoneInfo);

				meshSource->m_Skeleton = CreateScope<Skeleton>();
				Deserialize(&stream, *meshSource->m_Skeleton);
			}

			uint32_t animationCount;
			stream.ReadRaw(animationCount);
			for (uint32_t i = 0; i < animationCount; i++)
			{
				size_t hash;
				stream.ReadRaw<size_t>(hash);
				meshSource->m_Animations.emplace(hash, CreateScope<Animation>(meshSource->GetSkeleton()));
				Deserialize(&stream, *meshSource->m_Animations[hash]);
			}
		}

		if (!meshSource->m_Vertices.empty())
			meshSource->m_VertexBuffer = VertexBuffer::Create(meshSource->m_Vertices.data(), (uint32_t)(meshSource->m_Vertices.size() * sizeof(Vertex)));

		if (!meshSource->m_BoneInfluences.empty())
			meshSource->m_BoneInfluenceBuffer = VertexBuffer::Create(meshSource->m_BoneInfluences.data(), (uint32_t)(meshSource->m_BoneInfluences.size() * sizeof(BoneInfluence)));

		if(!meshSource->m_Indices.empty())
			meshSource->m_IndexBuffer = IndexBuffer::Create(meshSource->m_Indices.data(), (uint32_t)(meshSource->m_Indices.size() * sizeof(Index)));

		return meshSource;
	}
}

/// -------------------------------------------------------

