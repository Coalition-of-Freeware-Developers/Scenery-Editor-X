/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* mesh.h
* -------------------------------------------------------
* Created: 11/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <Math/includes/aabb.h>
#include "SceneryEditorX/asset/asset.h"
#include "SceneryEditorX/asset/asset_types.h"
#include "SceneryEditorX/asset/animation/mesh_skeleton.h"
#include "SceneryEditorX/renderer/buffers/index_buffer.h"
#include "SceneryEditorX/renderer/buffers/vertex_buffer.h"
#include "SceneryEditorX/scene/material.h"
//#include "SceneryEditorX/utils/filestreaming/filestream_reader.h"
//#include "SceneryEditorX/utils/filestreaming/filestream_writer.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

	struct Vertex
	{
		Vec3 Position;
		Vec3 Normal;
		Vec3 Tangent;
		Vec3 Binormal;
		Vec2 Texcoord;
	};

	struct BoneInfo
	{
		Mat4 InverseBindPose;
		uint32_t BoneIndex = Skeleton::NullIndex;

		/*
		static void Serialize(StreamWriter* serializer, const BoneInfo& instance)
		{
			serializer->WriteRaw(instance);
		}
		*/

		/*
		static void Deserialize(StreamReader* deserializer, BoneInfo& instance)
		{
			deserializer->ReadRaw(instance);
		}
		*/

	};

	struct BoneInfluence
	{
		uint32_t BoneInfoIndices[4] = { 0, 0, 0, 0 };
		float Weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		void AddBoneData(uint32_t boneInfoIndex, float weight)
		{
			if (weight < 0.0f || weight > 1.0f)
			{
				SEDX_CORE_WARN("Vertex bone weight is out of range. We will clamp it to [0, 1] (BoneID={0}, Weight={1})", boneInfoIndex, weight);
				weight = std::clamp(weight, 0.0f, 1.0f);
			}
			if (weight > 0.0f)
			{
				for (size_t i = 0; i < 4; i++)
				{
					if (Weights[i] == 0.0f)
					{
						BoneInfoIndices[i] = boneInfoIndex;
						Weights[i] = weight;
						return;
					}
				}

				/// Note: when importing from assimp we are passing aiProcess_LimitBoneWeights which automatically keeps only the top N (where N defaults to 4)
				///       bone weights (and normalizes the sum to 1), which is exactly what we want.
				///       So, we should never get here.
				SEDX_CORE_WARN("Vertex has more than four bones affecting it, extra bone influences will be discarded (BoneID={0}, Weight={1})", boneInfoIndex, weight);
			}
		}

		void NormalizeWeights()
		{
			float sumWeights = 0.0f;
			for (size_t i = 0; i < 4; i++)
			{
				sumWeights += Weights[i];
			}
			if (sumWeights > 0.0f)
			{
				for (size_t i = 0; i < 4; i++)
				{
					Weights[i] /= sumWeights;
				}
			}
		}

		/*
		static void Serialize(StreamWriter* serializer, const BoneInfluence& instance)
		{
			serializer->WriteRaw(instance);
		}
		*/

		/*
		static void Deserialize(StreamReader* deserializer, BoneInfluence& instance)
		{
			deserializer->ReadRaw(instance);
		}
		*/

	};

	static constexpr int NumAttributes = 5;

	struct Index
	{
        uint32_t V1;
        uint32_t V2;
        uint32_t V3;
	};

	static_assert(sizeof(Index) == 3 * sizeof(uint32_t));

	struct Triangle
	{
        Vertex V0;
        Vertex V1;
        Vertex V2;

		Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2) : V0(v0), V1(v1), V2(v2) {}
	};

	class Submesh
	{
	public:
		uint32_t BaseVertex = 0;
		uint32_t BaseIndex = 0;
		uint32_t MaterialIndex = 0;
		uint32_t IndexCount = 0;
		uint32_t VertexCount = 0;

		Mat4 Transform{ 1.0f }; /// World transform
		Mat4 LocalTransform{ 1.0f };
        AABB BoundingBox;

		std::string NodeName, MeshName;
		bool IsRigged = false;

		/*
		static void Serialize(StreamWriter* serializer, const Submesh& instance)
		{
			serializer->WriteRaw(instance.BaseVertex);
			serializer->WriteRaw(instance.BaseIndex);
			serializer->WriteRaw(instance.MaterialIndex);
			serializer->WriteRaw(instance.IndexCount);
			serializer->WriteRaw(instance.VertexCount);
			serializer->WriteRaw(instance.Transform);
			serializer->WriteRaw(instance.LocalTransform);
			serializer->WriteRaw(instance.BoundingBox);
			serializer->WriteString(instance.NodeName);
			serializer->WriteString(instance.MeshName);
			serializer->WriteRaw(instance.IsRigged);
		}
		*/

		/*
		static void Deserialize(StreamReader* deserializer, Submesh& instance)
		{
			deserializer->ReadRaw(instance.BaseVertex);
			deserializer->ReadRaw(instance.BaseIndex);
			deserializer->ReadRaw(instance.MaterialIndex);
			deserializer->ReadRaw(instance.IndexCount);
			deserializer->ReadRaw(instance.VertexCount);
			deserializer->ReadRaw(instance.Transform);
			deserializer->ReadRaw(instance.LocalTransform);
			deserializer->ReadRaw(instance.BoundingBox);
			deserializer->ReadString(instance.NodeName);
			deserializer->ReadString(instance.MeshName);
			deserializer->ReadRaw(instance.IsRigged);
		}
		*/

	};

	struct MeshNode
	{
		uint32_t Parent = 0xffffffff;
		std::vector<uint32_t> Children;
		std::vector<uint32_t> Submeshes;

		std::string Name;
		Mat4 LocalTransform;

		inline bool IsRoot() const { return Parent == 0xffffffff; }

		/*
		static void Serialize(StreamWriter* serializer, const MeshNode& instance)
		{
			serializer->WriteRaw(instance.Parent);
			serializer->WriteArray(instance.Children);
			serializer->WriteArray(instance.Submeshes);
			serializer->WriteString(instance.Name);
			serializer->WriteRaw(instance.LocalTransform);
		}
		*/

		/*
		static void Deserialize(StreamReader* deserializer, MeshNode& instance)
		{
			deserializer->ReadRaw(instance.Parent);
			deserializer->ReadArray(instance.Children);
			deserializer->ReadArray(instance.Submeshes);
			deserializer->ReadString(instance.Name);
			deserializer->ReadRaw(instance.LocalTransform);
		}
		*/

	};

	/// MeshSource is a representation of an actual asset file on disk
	/// Meshes are created from MeshSource
	class MeshSource : public Asset
	{
	public:
		MeshSource() = default;
		MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const Mat4& transform);
		MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const std::vector<Submesh>& submeshes);
		virtual ~MeshSource();

		void DumpVertexBuffer();

		std::vector<Submesh>& GetSubmeshes() { return m_Submeshes; }
		const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }

		const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
		const std::vector<Index>& GetIndices() const { return m_Indices; }

		bool HasSkeleton() const { return (bool)m_Skeleton; }
		bool IsSubmeshRigged(uint32_t submeshIndex) const { return m_Submeshes[submeshIndex].IsRigged; }
		const Skeleton* GetSkeleton() const { return m_Skeleton.get(); }
		/// Sometimes the nodes between the scene root and the skeleton root can have some transforms (e.g. rotations from various axis orientation conventions)
		/// Usually this doesn't matter (because the skin bound to the skeleton is already transformed by these nodes)
		/// However, if we just want to render the skeleton without any skin, then we need to know what this is)
		const Mat4& GetSkeletonTransform() const { return m_SkeletonTransform; }

		bool IsCompatibleSkeleton(const std::string_view animationName, const Skeleton& skeleton) const;

		std::vector<std::string> GetAnimationNames() const;
		/// note: can return nullptr (e.g. if named animation does not exist, or something goes wrong retrieving the animation)
		const Animation* GetAnimation(const std::string& animationName, const Skeleton& skeleton, bool extractRootMotion, uint32_t rootBoneIndex, const Bool3& rootTranslationMask, const Bool3& rootRotationMask, bool discardRootMotion) const;
		const std::vector<BoneInfluence>& GetBoneInfluences() const { return m_BoneInfluences; }

		std::vector<AssetHandle>& GetMaterials() { return m_Materials; }
		const std::vector<AssetHandle>& GetMaterials() const { return m_Materials; }
		const std::string& GetFilePath() const { return m_FilePath; }

		const std::vector<Triangle> GetTriangleCache(uint32_t index) const { return m_TriangleCache.at(index); }

		Ref<VertexBuffer> GetVertexBuffer() { return m_VertexBuffer; }
		Ref<VertexBuffer> GetBoneInfluenceBuffer() { return m_BoneInfluenceBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() { return m_IndexBuffer; }

		static AssetType GetStaticType() { return AssetType::MeshSource; }
		virtual ObjectType GetAssetType() const override { return static_cast<ObjectType>(GetStaticType()); }

		const AABB& GetBoundingBox() const { return m_BoundingBox; }

		const MeshNode& GetRootNode() const { return m_Nodes[0]; }
		const std::vector<MeshNode>& GetNodes() const { return m_Nodes; }

	private:
		std::vector<Submesh> m_Submeshes;

		Ref<VertexBuffer> m_VertexBuffer;
		Ref<VertexBuffer> m_BoneInfluenceBuffer;
		Ref<IndexBuffer> m_IndexBuffer;

		std::vector<Vertex> m_Vertices;
		std::vector<Index> m_Indices;

		std::vector<BoneInfluence> m_BoneInfluences;
		std::vector<BoneInfo> m_BoneInfo;
		Scope<Skeleton> m_Skeleton;
		Mat4 m_SkeletonTransform{ 1.0f };
		std::vector<std::string> m_AnimationNames;
		mutable std::unordered_map<size_t, Scope<Animation>> m_Animations;

		std::vector<AssetHandle> m_Materials;

		std::unordered_map<uint32_t, std::vector<Triangle>> m_TriangleCache;

        AABB m_BoundingBox;

		std::string m_FilePath;

		std::vector<MeshNode> m_Nodes;

		/// TEMP
		bool m_Runtime = false;

		friend class Scene;
		friend class SceneRenderer;
		friend class Renderer;
		friend class SceneHierarchyPanel;
		friend class MeshViewerPanel;
		friend class Mesh;
		friend class AssimpMeshImporter;
		friend class MeshRuntimeSerializer;
	};

	/// Dynamic Mesh - supports skeletal animation and retains hierarchy
	class Mesh : public Asset
	{
	public:
		explicit Mesh(AssetHandle meshSource, bool generateColliders);
		Mesh(AssetHandle meshSource, const std::vector<uint32_t>& submeshes, bool generateColliders);
		virtual ~Mesh() = default;

		virtual void OnDependencyUpdated(AssetHandle handle);

		const std::vector<uint32_t>& GetSubmeshes() const { return m_Submeshes; }

		// Pass in an empty vector to set ALL submeshes for MeshSource
		void SetSubmeshes(const std::vector<uint32_t>& submeshes, Ref<MeshSource> meshSource);

		AssetHandle GetMeshSource() const { return m_MeshSource; }
		void SetMeshAsset(const AssetHandle &meshSource) { m_MeshSource = meshSource; }

		Ref<MaterialTable> GetMaterials() const { return m_Materials; }

		bool ShouldGenerateColliders() const { return m_GenerateColliders; }

		static AssetType GetStaticType() { return AssetType::Mesh; }
		virtual ObjectType GetAssetType() const override { return static_cast<ObjectType>(GetStaticType()); }

		const Skeleton* GetSkeleton() const;

	private:
		AssetHandle m_MeshSource;
		std::vector<uint32_t> m_Submeshes; // TODO(Yan): physics/render masks

		/// Materials
		Ref<MaterialTable> m_Materials;

		bool m_GenerateColliders = false; /// should we generate physics colliders when (re)loading this mesh?

		friend class Scene;
		friend class Renderer;
		friend class SceneHierarchyPanel;
		friend class MeshViewerPanel;
	};

	/// Static Mesh - no skeletal animation, flattened hierarchy
	class StaticMesh : public Asset
	{
	public:
		explicit StaticMesh(AssetHandle meshSource, bool generateColliders);
		StaticMesh(AssetHandle meshSource, const std::vector<uint32_t>& submeshes, bool generateColliders);
		virtual ~StaticMesh() = default;

	    virtual void OnDependencyUpdated(AssetHandle handle);

		const std::vector<uint32_t>& GetSubmeshes() const { return m_Submeshes; }

		/// Pass in an empty vector to set ALL submeshes for MeshSource
		void SetSubmeshes(const std::vector<uint32_t>& submeshes, Ref<MeshSource> meshSourceAsset);

		AssetHandle GetMeshSource() const { return m_MeshSource; }
		void SetMeshAsset(AssetHandle meshSource) { m_MeshSource = meshSource; }

		Ref<MaterialTable> GetMaterials() const { return m_Materials; }

		bool ShouldGenerateColliders() const { return m_GenerateColliders; }

		static AssetType GetStaticType() { return AssetType::StaticMesh; }
		virtual ObjectType GetAssetType() const override { return static_cast<ObjectType>(GetStaticType()); }
	private:
		AssetHandle m_MeshSource;
		std::vector<uint32_t> m_Submeshes; /// TODO: physics/render masks

		/// Materials
		Ref<MaterialTable> m_Materials;

		bool m_GenerateColliders = false; /// should we generate physics colliders when (re)loading this static mesh?

		friend class Scene;
		friend class Renderer;
		friend class SceneHierarchyPanel;
		friend class MeshViewerPanel;
	};


}

/// -------------------------------------------------------
