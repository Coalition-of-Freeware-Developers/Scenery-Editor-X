/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* mesh_file.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <cstdint>
#include <Math/includes/aabb.h>

/// -----------------------------------------------------

namespace SceneryEditorX
{

	struct MeshSourceFile
	{
		enum class MeshFlags : uint32_t
		{
			HasMaterials = BIT(0),
			HasAnimation = BIT(1),
			HasSkeleton = BIT(2)
		};

		struct Metadata
		{
			uint32_t Flags;
            Utils::AABB BoundingBox;

			uint64_t NodeArrayOffset;
			uint64_t NodeArraySize;

			uint64_t SubmeshArrayOffset;
			uint64_t SubmeshArraySize;

			uint64_t MaterialArrayOffset;
			uint64_t MaterialArraySize;

			uint64_t VertexBufferOffset;
			uint64_t VertexBufferSize;

			uint64_t IndexBufferOffset;
			uint64_t IndexBufferSize;

			uint64_t AnimationDataOffset;
			uint64_t AnimationDataSize;
		};

		struct FileHeader
		{
			const char HEADER[3] = { 'e','d','X' };
			uint32_t Version = 1;
			// other metadata?
		};

		FileHeader Header;
		Metadata Data;
	};


}
