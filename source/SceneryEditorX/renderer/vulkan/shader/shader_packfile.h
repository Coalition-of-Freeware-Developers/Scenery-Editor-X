/**
 * -------------------------------------------------------
 * RailNex Train Simulator
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 RailNex
 * -------------------------------------------------------
 * shader_packfile.h
 * -------------------------------------------------------
 * rnxForge.dll
 * -------------------------------------------------------
 * Created: 23/06/2026
 * -------------------------------------------------------
 */
#pragma once
#include <map>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX 
{

	/**
	 * @struct ShaderPackFile
	 * @brief Represents a shader pack file containing shader programs and modules.
	 */
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

		/**
		 * @struct ShaderData 
		 * @brief Represents the data of a shader in a shader pack file.
		 */
		struct ShaderData
		{
			uint8_t stage;
			ShaderReflectionInfo reflectionInfo;
			void* data;
		};

		/**
		 * @struct ShaderModuleInfo 
		 * @brief Represents the information of a shader module in a shader pack file.
		 */
		struct ShaderModuleInfo
		{
			uint64_t packedOffset	= 0;
			uint64_t packedSize		= 0; // size of data only
			uint8_t version			= 0;
			uint16_t stage			= 0; // GFX::ShaderType stored as raw flag value
			uint32_t flags			= 0;

			static void Serialize(StreamWriter* writer, const ShaderModuleInfo& info)
			{
				writer->WriteRaw(info.packedOffset);
				writer->WriteRaw(info.packedSize);
				writer->WriteRaw(info.version);
				writer->WriteRaw(info.stage);
				writer->WriteRaw(info.flags);
			}

			static void Deserialize(StreamReader* reader, ShaderModuleInfo& info)
			{
				reader->ReadRaw(info.packedOffset);
				reader->ReadRaw(info.packedSize);
				reader->ReadRaw(info.version);
				reader->ReadRaw(info.stage);
				reader->ReadRaw(info.flags);
			}
		};

		/**
		 * @struct ShaderProgramInfo 
		 * @brief Represents the information of a shader program in a shader pack file.
		 */
		struct ShaderProgramInfo
		{
			uint64_t reflectionDataOffset;
			std::vector<uint32_t> moduleIndices;
		};

		/**
		 * @struct ShaderIndex 
		 * @brief Represents the index of shaders in a shader pack file.
		 */
		struct ShaderIndex
		{
			std::map<uint32_t, ShaderProgramInfo> shaderPrograms; // Hashed shader name/path
			std::vector<ShaderModuleInfo> shaderModules;          // vector of shader modules

			/**
			 * @brief Calculates the size requirements for a given number of shader programs and modules.
			 * @param programCount The number of shader programs.
			 * @param moduleCount The number of shader modules.
			 * @return The total size required to store the shader programs and modules.
			 */
			static uint64_t CalculateSizeRequirements(uint32_t programCount, uint32_t moduleCount)
			{
				return (sizeof(uint32_t) + sizeof(ShaderProgramInfo)) * programCount + sizeof(ShaderModuleInfo) * moduleCount;
			}
		};

		/**
		 * @struct FileHeader
		 * @brief Represents the header of a shader pack file.
		 */
		struct FileHeader
		{
			char HEADER[3] = { 'S','S','B'}; // SceneryEditorX  Shader Binary
			uint32_t version = 1;
			uint32_t shaderProgramCount = 0;
			uint32_t shaderModuleCount = 0;
		};

		FileHeader header;
		ShaderIndex index;
		ShaderData* data;

	};

}

// -------------------------------------------------------
