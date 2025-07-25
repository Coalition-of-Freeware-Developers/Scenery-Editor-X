/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shader_pack_file.h
* -------------------------------------------------------
* Created: 9/7/2025
* -------------------------------------------------------
*/
#pragma once
//#include <SceneryEditorX/filestreaming/filestream_writer.h>
#include <SceneryEditorX/serialization/serializer_reader.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/*
	struct ShaderPackFile
	{
		struct ShaderReflectionInfo
		{
			// Uniform Buffers
			// Storage Buffers
			// Push Constant Buffers
			// Sampled Images
			// Storage Images
		};

		/*
		struct ShaderData
		{
			uint8_t Stage;
			ShaderReflectionInfo ReflectionInfo;
			void* Data;
		};
		#1#

		/*
		struct ShaderModuleInfo
		{
			uint64_t PackedOffset;
			uint64_t PackedSize; // size of data only
			uint8_t Version;
			uint8_t Stage;
			uint32_t Flags = 0;
			static void Serialize(StreamWriter* writer, const ShaderModuleInfo& info) { writer->WriteRaw(info); }
			static void Deserialize(SerializeReader* reader, ShaderModuleInfo& info) { reader->ReadRaw(info); }
		};
		#1#

		struct ShaderProgramInfo
		{
			uint64_t ReflectionDataOffset;
			std::vector<uint32_t> ModuleIndices;
		};

		struct ShaderIndex
		{
			std::map<uint32_t, ShaderProgramInfo> ShaderPrograms; // Hashed shader name/path
			std::vector<ShaderModuleInfo> ShaderModules;

			static uint64_t CalculateSizeRequirements(const uint32_t programCount, const uint32_t moduleCount)
			{
				return (sizeof(uint32_t) + sizeof(ShaderProgramInfo)) * programCount + sizeof(ShaderModuleInfo) * moduleCount;
			}
		};

		struct FileHeader
		{
			char HEADER[3] = { 'X','S','A' };
			uint32_t Version = 1;
			uint32_t ShaderProgramCount, ShaderModuleCount;
		};

		FileHeader Header;
		ShaderIndex Index;
		ShaderData* Data;

	};
	*/

}

/// -------------------------------------------------------
