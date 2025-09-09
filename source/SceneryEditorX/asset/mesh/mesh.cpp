/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* mesh.cpp
* -------------------------------------------------------
* Created: 11/8/2025
* -------------------------------------------------------
*/
//#include "mesh.h"
//#include "SceneryEditorX/asset/managers/asset_manager.h"
//#include "SceneryEditorX/project/project.h"

/// -------------------------------------------------------

/*
namespace SceneryEditorX
{

#define MESH_DEBUG_LOG 0
#if MESH_DEBUG_LOG
#define SEDX_MESH_LOG(...) SEDX_CORE_TRACE_TAG("Mesh", __VA_ARGS__)
#define SEDX_MESH_ERROR(...) SEDX_CORE_ERROR_TAG("Mesh", __VA_ARGS__)
#else
#define SEDX_MESH_LOG(...)
#define SEDX_MESH_ERROR(...)
#endif

	////////////////////////////////////////////////////////
	/// MeshSource /////////////////////////////////////////
	////////////////////////////////////////////////////////
	MeshSource::MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const Mat4& transform) : m_Vertices(vertices), m_Indices(indices)
	{
		/// Generate a new asset handle
		Handle = {};

		Submesh& submesh = m_Submeshes.emplace_back();
		submesh.BaseVertex = 0;
		submesh.BaseIndex = 0;
		submesh.VertexCount = (uint32_t)m_Vertices.size();
		submesh.IndexCount = (uint32_t)indices.size() * 3u;
		submesh.Transform = transform;

		m_VertexBuffer = CreateRef<VertexBuffer>(m_Vertices.data(), (uint32_t)(m_Vertices.size() * sizeof(Vertex)));
		m_IndexBuffer = CreateRef<IndexBuffer>(m_Indices.data(), (uint32_t)(m_Indices.size() * sizeof(Index)));

		m_TriangleCache[0].reserve(indices.size());
		for (const Index& index : indices)
			m_TriangleCache[0].emplace_back(vertices[index.V1], vertices[index.V2], vertices[index.V3]);

		// Calculate bounding box
		m_BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
		m_BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for (size_t i = 0; i < m_Vertices.size(); i++)
		{
			const Vertex& vertex = m_Vertices[i];
			m_BoundingBox.Min.x = Math::Min(vertex.Position.x, m_BoundingBox.Min.x);
			m_BoundingBox.Min.y = Math::Min(vertex.Position.y, m_BoundingBox.Min.y);
			m_BoundingBox.Min.z = Math::Min(vertex.Position.z, m_BoundingBox.Min.z);
			m_BoundingBox.Max.x = Math::Max(vertex.Position.x, m_BoundingBox.Max.x);
			m_BoundingBox.Max.y = Math::Max(vertex.Position.y, m_BoundingBox.Max.y);
			m_BoundingBox.Max.z = Math::Max(vertex.Position.z, m_BoundingBox.Max.z);
		}

		submesh.BoundingBox = m_BoundingBox;
	}

	MeshSource::MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const std::vector<Submesh>& submeshes) : m_Submeshes(submeshes), m_Vertices(vertices), m_Indices(indices)
	{
		/// Generate a new asset handle
		Handle = {};

		m_VertexBuffer = CreateRef<VertexBuffer>(m_Vertices.data(), (uint32_t)(m_Vertices.size() * sizeof(Vertex)));
        m_IndexBuffer = CreateRef<IndexBuffer>(m_Indices.data(), (uint32_t)(m_Indices.size() * sizeof(Index)));

		/// Calculate bounding box
		m_BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
		m_BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for (size_t i = 0; i < m_Vertices.size(); i++)
		{
			const Vertex& vertex = m_Vertices[i];
			m_BoundingBox.Min.x = Math::Min(vertex.Position.x, m_BoundingBox.Min.x);
			m_BoundingBox.Min.y = Math::Min(vertex.Position.y, m_BoundingBox.Min.y);
			m_BoundingBox.Min.z = Math::Min(vertex.Position.z, m_BoundingBox.Min.z);
			m_BoundingBox.Max.x = Math::Max(vertex.Position.x, m_BoundingBox.Max.x);
			m_BoundingBox.Max.y = Math::Max(vertex.Position.y, m_BoundingBox.Max.y);
			m_BoundingBox.Max.z = Math::Max(vertex.Position.z, m_BoundingBox.Max.z);
		}
	}

	MeshSource::~MeshSource() = default;

    static std::string LevelToSpaces(uint32_t level)
	{
		std::string result;
		for (uint32_t i = 0; i < level; i++)
			result += "--";
		return result;
	}

	void MeshSource::DumpVertexBuffer()
	{
		/// TODO: Convert to ImGui
		SEDX_MESH_LOG("------------------------------------------------------");
		SEDX_MESH_LOG("Vertex Buffer Dump");
		SEDX_MESH_LOG("Mesh: {0}", m_FilePath);
		for (size_t i = 0; i < m_Vertices.size(); i++)
		{
			auto& vertex = m_Vertices[i];
			SEDX_MESH_LOG("Vertex: {0}", i);
			SEDX_MESH_LOG("Position: {0}, {1}, {2}", vertex.Position.x, vertex.Position.y, vertex.Position.z);
			SEDX_MESH_LOG("Normal: {0}, {1}, {2}", vertex.Normal.x, vertex.Normal.y, vertex.Normal.z);
			SEDX_MESH_LOG("Binormal: {0}, {1}, {2}", vertex.Binormal.x, vertex.Binormal.y, vertex.Binormal.z);
			SEDX_MESH_LOG("Tangent: {0}, {1}, {2}", vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z);
			SEDX_MESH_LOG("TexCoord: {0}, {1}", vertex.Texcoord.x, vertex.Texcoord.y);
			SEDX_MESH_LOG("--");
		}
		SEDX_MESH_LOG("------------------------------------------------------");
	}


	/// TODO: this is temporary and will eventually be replaced with some kind of skeleton retargeting

	/*
	bool MeshSource::IsCompatibleSkeleton(const std::string_view animationName, const Skeleton& skeleton) const
	{
		if (!m_Skeleton)
		{
			SEDX_CORE_VERIFY(!m_Runtime);
			auto path = Project::GetEditorAssetManager()->GetFileSystemPath(Handle);
			AssimpMeshImporter importer(path);
			return importer.IsCompatibleSkeleton(animationName, skeleton);
		}

		return m_Skeleton->GetBoneNames() == skeleton.GetBoneNames();
	}
	#1#

	std::vector<std::string> MeshSource::GetAnimationNames() const
	{
		return m_AnimationNames;
	}


	/*
	const Animation* MeshSource::GetAnimation(const std::string& animationName, const Skeleton& skeleton, bool extractRootMotion, uint32_t rootBoneIndex, const Bool3& rootTranslationMask, const Bool3& rootRotationMask, bool discardRootMotion) const
	{
		std::size_t hash = 0;
		auto& animation = m_Animations[hash];
		if (animation)
		{
			if (!animation->GetSkeleton())
			{
				animation->SetSkeleton(&skeleton);
			}
		}
		else
		{
			/// Deferred load of animations.
			/// We cannot load them earlier (e.g. in MeshSource constructor) for two reasons:
			/// 1) Assimp does not import bones (and hence no skeleton) if the mesh source file contains only animations (and no skin)
			///    This means we need to wait until we know what the skeleton is before we can load the animations.
			/// 2) We don't have any way to pass the root motion parameters to the mesh source constructor
			SEDX_CORE_VERIFY(!m_Runtime);
			auto path = Project::GetEditorAssetManager()->GetFileSystemPath(Handle);
			AssimpMeshImporter importer(path);
			importer.ImportAnimation(animationName, skeleton, extractRootMotion, rootBoneIndex, rootTranslationMask, rootRotationMask, discardRootMotion, animation);
		}
		return animation.get(); /// Note: could be nullptr (e.g. if the import above failed.)
	}
	#1#


	Mesh::Mesh(AssetHandle meshSource, bool generateColliders) : m_MeshSource(meshSource), m_GenerateColliders(generateColliders)
	{
		/// Generate a new asset handle
		Handle = {};

		/// Make sure to create material table even if meshsource asset cannot be retrieved
		/// (this saves having to keep checking mesh->m_Materials is not null elsewhere in the code)
		m_Materials = CreateRef<MaterialTable>(0);

		/*
		if (auto meshSourceAsset = AssetManager::GetAsset<MeshSource>(meshSource); meshSourceAsset)
		{
			SetSubmeshes({}, meshSourceAsset);

			const std::vector<AssetHandle>& meshMaterials = meshSourceAsset->GetMaterials();
			for (size_t i = 0; i < meshMaterials.size(); i++)
				m_Materials->SetMaterial((uint32_t)i, meshMaterials[i]);
		}
		#1#

	}

	Mesh::Mesh(AssetHandle meshSource, const std::vector<uint32_t>& submeshes, bool generateColliders) : m_MeshSource(meshSource), m_GenerateColliders(generateColliders)
	{
		/// Generate a new asset handle
		Handle = {};

		/// Make sure to create material table even if meshsource asset cannot be retrieved
		/// (this saves having to keep checking mesh->m_Materials is not null elsewhere in the code)
		m_Materials = CreateRef<MaterialTable>(0);

		/*
		if (auto meshSourceAsset = AssetManager::GetAsset<MeshSource>(meshSource); meshSourceAsset)
		{
			SetSubmeshes(submeshes, meshSourceAsset);

			const std::vector<AssetHandle>& meshMaterials = meshSourceAsset->GetMaterials();
			for (size_t i = 0; i < meshMaterials.size(); i++)
				m_Materials->SetMaterial((uint32_t)i, meshMaterials[i]);
		}
		#1#

	}

	/*
	void Mesh::OnDependencyUpdated(AssetHandle handle)
	{
		Project::GetAssetManager()->ReloadDataAsync(Handle);
	}
	#1#

	void Mesh::SetSubmeshes(const std::vector<uint32_t>& submeshes, Ref<MeshSource> meshSource)
	{
		if (!submeshes.empty())
		{
			m_Submeshes = submeshes;
		}
		else
		{
			const auto& submeshes = meshSource->GetSubmeshes();
			m_Submeshes.resize(submeshes.size());
			for (uint32_t i = 0; i < submeshes.size(); i++)
				m_Submeshes[i] = i;
		}
	}

	const SceneryEditorX::Skeleton* Mesh::GetSkeleton() const
	{
		/*
		if (auto meshSourceAsset = AssetManager::GetAsset<MeshSource>(m_MeshSource); meshSourceAsset)
            return meshSourceAsset->GetSkeleton();
        #1#

        return nullptr;
	}

	////////////////////////////////////////////////////////
	/// StaticMesh /////////////////////////////////////////
	////////////////////////////////////////////////////////

	StaticMesh::StaticMesh(AssetHandle meshSource, bool generateColliders) : m_MeshSource(meshSource), m_GenerateColliders(generateColliders)
	{
		/// Generate a new asset handle
		Handle = {};

		/// Make sure to create material table even if meshsource asset cannot be retrieved
		/// (this saves having to keep checking mesh->m_Materials is not null elsewhere in the code)
        m_Materials = CreateRef<MaterialTable>(0);

		/*
		if(auto meshSourceAsset = AssetManager::GetAsset<MeshSource>(meshSource); meshSourceAsset)
		{
			SetSubmeshes({}, meshSourceAsset);

			const std::vector<AssetHandle>& meshMaterials = meshSourceAsset->GetMaterials();
			uint32_t numMaterials = static_cast<uint32_t>(meshMaterials.size());
			for (uint32_t i = 0; i < numMaterials; i++)
				m_Materials->SetMaterial(i, meshMaterials[i]);
		}
		#1#

	}

	StaticMesh::StaticMesh(AssetHandle meshSource, const std::vector<uint32_t>& submeshes, bool generateColliders) : m_MeshSource(meshSource), m_GenerateColliders(generateColliders)
	{
		/// Generate a new asset handle
		Handle = {};

		/// Make sure to create material table even if meshsource asset cannot be retrieved
		/// (this saves having to keep checking mesh->m_Materials is not null elsewhere in the code)
        m_Materials = CreateRef<MaterialTable>(0);

		/*
		if (auto meshSourceAsset = AssetManager::GetAsset<MeshSource>(meshSource); meshSourceAsset)
		{
			SetSubmeshes(submeshes, meshSourceAsset);

			const std::vector<AssetHandle>& meshMaterials = meshSourceAsset->GetMaterials();
			uint32_t numMaterials = static_cast<uint32_t>(meshMaterials.size());
			for (uint32_t i = 0; i < numMaterials; i++)
				m_Materials->SetMaterial(i, meshMaterials[i]);
		}
		#1#

	}

	/*
	void StaticMesh::OnDependencyUpdated(AssetHandle)
	{
		Project::GetAssetManager()->ReloadDataAsync(Handle);
	}
	#1#

	void StaticMesh::SetSubmeshes(const std::vector<uint32_t>& submeshes, Ref<MeshSource> meshSource)
	{
		if (!submeshes.empty())
		{
			m_Submeshes = submeshes;
		}
		else
		{
			const auto& submeshes = meshSource->GetSubmeshes();
			m_Submeshes.resize(submeshes.size());
			for (uint32_t i = 0; i < submeshes.size(); i++)
				m_Submeshes[i] = i;
		}
	}

}
*/

/// -------------------------------------------------------
