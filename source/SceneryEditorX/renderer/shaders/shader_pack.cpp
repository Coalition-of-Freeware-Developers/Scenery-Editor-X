/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shader_pack.cpp
* -------------------------------------------------------
* Created: 9/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/shaders/shader_pack.h>
#include <SceneryEditorX/utils/filestreaming/file_streaming.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace Utils
	{
		enum class ShaderStage : uint8_t
		{
			None = 0,
			Vertex = 1,
			Fragment = 2,
			Compute = 3,
            TessellationControl = 4,
			TessellationEvaluation = 5,
			Geometry = 6,
			AllGraphics = 7,
            All = 8,
            RayGen = 9,
            AnyHit = 10,
			ClosestHit = 11,
			Miss = 12,
			Intersection = 13,
			Callable = 14,
			Task = 15,
			Mesh = 16,
			SubpassShading = 17,
			ClusterCulling = 18
		};

        static VkShaderStageFlagBits ShaderStageToVkShaderStage(const ShaderStage stage)
		{
			switch (stage)
			{
				case ShaderStage::Vertex:					return VK_SHADER_STAGE_VERTEX_BIT;
				case ShaderStage::Fragment:					return VK_SHADER_STAGE_FRAGMENT_BIT;
				case ShaderStage::Compute:					return VK_SHADER_STAGE_COMPUTE_BIT;
                case ShaderStage::TessellationControl:		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				case ShaderStage::TessellationEvaluation:	return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                case ShaderStage::Geometry:					return VK_SHADER_STAGE_GEOMETRY_BIT;
                case ShaderStage::AllGraphics:				return VK_SHADER_STAGE_ALL_GRAPHICS;
                case ShaderStage::All:						return VK_SHADER_STAGE_ALL;
				case ShaderStage::RayGen:					return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
				case ShaderStage::AnyHit:					return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				case ShaderStage::ClosestHit:				return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
                case ShaderStage::Miss:						return VK_SHADER_STAGE_MISS_BIT_KHR;
                case ShaderStage::Intersection:				return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
                case ShaderStage::Callable:					return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
                case ShaderStage::Task:						return VK_SHADER_STAGE_TASK_BIT_EXT;
				case ShaderStage::Mesh:						return VK_SHADER_STAGE_MESH_BIT_EXT;
				case ShaderStage::SubpassShading:			return VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI;
				case ShaderStage::ClusterCulling:			return VK_SHADER_STAGE_CLUSTER_CULLING_BIT_HUAWEI;
            case ShaderStage::None:
                break;
            }
			SEDX_CORE_VERIFY(false);
			return (VkShaderStageFlagBits)0;
		}

        static ShaderStage ShaderStageFromVkShaderStage(VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT:					return ShaderStage::Vertex;
				case VK_SHADER_STAGE_FRAGMENT_BIT:					return ShaderStage::Fragment;
				case VK_SHADER_STAGE_COMPUTE_BIT:					return ShaderStage::Compute;
                case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:		return ShaderStage::TessellationControl;
                case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:	return ShaderStage::TessellationEvaluation;
				case VK_SHADER_STAGE_GEOMETRY_BIT:					return ShaderStage::Geometry;
				case VK_SHADER_STAGE_ALL_GRAPHICS:					return ShaderStage::AllGraphics;
                case VK_SHADER_STAGE_ALL:							return ShaderStage::All;
			    case VK_SHADER_STAGE_RAYGEN_BIT_KHR:				return ShaderStage::RayGen;
				case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:				return ShaderStage::AnyHit;
				case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:			return ShaderStage::ClosestHit;
				case VK_SHADER_STAGE_MISS_BIT_KHR:					return ShaderStage::Miss;
				case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:			return ShaderStage::Intersection;
				case VK_SHADER_STAGE_CALLABLE_BIT_KHR:				return ShaderStage::Callable;
				case VK_SHADER_STAGE_TASK_BIT_EXT:					return ShaderStage::Task;
                case VK_SHADER_STAGE_MESH_BIT_EXT:					return ShaderStage::Mesh;
                case VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI:	return ShaderStage::SubpassShading;
                case VK_SHADER_STAGE_CLUSTER_CULLING_BIT_HUAWEI:	return ShaderStage::ClusterCulling;
            }
			SEDX_CORE_VERIFY(false);
			return (ShaderStage)0;
		}
	}

	/*
	ShaderPack::ShaderPack(const std::filesystem::path& path) : m_Path(path)
	{
		/// Read index
		FileStreamReader serializer(path);
		if (!serializer)
			return;
		serializer.ReadRaw(m_File.Header);
		if (memcmp(m_File.Header.HEADER, "SEDX", 4) != 0)
			return;
		m_Loaded = true;
		for (uint32_t i = 0; i < m_File.Header.ShaderProgramCount; i++)
		{
			uint32_t key;
			serializer.ReadRaw(key);
			auto&[ReflectionDataOffset, ModuleIndices] = m_File.Index.ShaderPrograms[key];
			serializer.ReadRaw(ReflectionDataOffset);
			serializer.ReadArray(ModuleIndices);
		}
		serializer.GetStreamPosition();
		serializer.ReadArray(m_File.Index.ShaderModules, m_File.Header.ShaderModuleCount);
	}
	*/

	/*
	bool ShaderPack::Contains(const std::string_view name) const
	{
        return m_File.Index.ShaderPrograms.contains(Hash::GenerateFNVHash(std::string(name)));
	}

	Ref<Shader> ShaderPack::LoadShader(const std::string_view name)
	{
        const uint32_t nameHash = Hash::GenerateFNVHash(std::string(name));
		SEDX_CORE_VERIFY(Contains(name));
		const auto&[ReflectionDataOffset, ModuleIndices] = m_File.Index.ShaderPrograms.at(nameHash);
		FileStreamReader serializer(m_Path);
		serializer.SetStreamPosition(ReflectionDataOffset);
		///< Debug only
		std::string shaderName;
		{
			std::string path(name);
			size_t found = path.find_last_of("/\\");
			shaderName = found != std::string::npos ? path.substr(found + 1) : path;
			found = shaderName.find_last_of('.');
			shaderName = found != std::string::npos ? shaderName.substr(0, found) : name;
		}

		Ref<Shader> vulkanShader = CreateRef<Shader>();
		vulkanShader->m_Name = shaderName;
		vulkanShader->m_AssetPath = name;
		//vulkanShader->TryReadReflectionData(&serializer);
		//vulkanShader->m_DisableOptimization =
		std::map<VkShaderStageFlagBits, std::vector<uint32_t>> shaderModules;
		for (const uint32_t index : ModuleIndices)
		{
			const auto& info = m_File.Index.ShaderModules[index];
			auto& moduleData = shaderModules[Utils::ShaderStageToVkShaderStage((Utils::ShaderStage)info.Stage)];
			serializer.SetStreamPosition(info.PackedOffset);
			serializer.ReadArray(moduleData, (uint32_t)info.PackedSize);
		}

		serializer.SetStreamPosition(ReflectionDataOffset);
		vulkanShader->TryReadReflectionData(&serializer);
		vulkanShader->LoadAndCreateShaders(shaderModules);
		vulkanShader->CreateDescriptors();
		//Renderer::AcknowledgeParsedGlobalMacros(compiler->GetAcknowledgedMacros(), vulkanShader);
		//Renderer::OnShaderReloaded(vulkanShader->GetHash());
		return vulkanShader;
	}
	*/

	/*
	Ref<ShaderPack> ShaderPack::CreateFromLibrary(const Ref<ShaderLibrary> &shaderLibrary, const std::filesystem::path& path)
	{
		Ref<ShaderPack> shaderPack = CreateRef<ShaderPack>();
		const auto& shaderMap = shaderLibrary->GetShaders();
		auto& shaderPackFile = shaderPack->m_File;
		shaderPackFile.Header.Version = 1;
		shaderPackFile.Header.ShaderProgramCount = (uint32_t)shaderMap.size();
		shaderPackFile.Header.ShaderModuleCount = 0;
		///< Determine number of modules (per shader)
		///< NOTE: this currently doesn't care about duplicated modules, but it should (eventually, not that important atm)
		uint32_t shaderModuleIndex = 0;
		uint32_t shaderModuleIndexArraySize = 0;
		for (const auto &shader : shaderMap | std::views::values)
		{
			const Ref<Shader> vulkanShader = shader.As<Shader>();
			const auto& shaderData = vulkanShader->m_ShaderData;
			shaderPackFile.Header.ShaderModuleCount += (uint32_t)shaderData.size();
			auto&[ReflectionDataOffset, ModuleIndices] = shaderPackFile.Index.ShaderPrograms[(uint32_t)vulkanShader->GetHash()];
			for (int i = 0; std::cmp_less(i, shaderData.size()); i++)
				ModuleIndices.emplace_back(shaderModuleIndex++);

			shaderModuleIndexArraySize += sizeof(uint32_t); ///< size
			shaderModuleIndexArraySize += (uint32_t)shaderData.size() * sizeof(uint32_t); ///< indices
		}

		const uint32_t shaderProgramIndexSize = shaderPackFile.Header.ShaderProgramCount * (sizeof(std::map<uint32_t, ShaderPackFile::ShaderProgramInfo>::key_type) + sizeof(ShaderPackFile::ShaderProgramInfo::ReflectionDataOffset)) + shaderModuleIndexArraySize;
		FileStreamWriter serializer(path);

		///< Write header
		serializer.WriteRaw<ShaderPackFile::FileHeader>(shaderPackFile.Header);
		/// ===============
		///< Write index
		/// ===============
		///< Write dummy data for shader programs
		const uint64_t shaderProgramIndexPos = serializer.GetStreamPosition();
		serializer.WriteZero(shaderProgramIndexSize);
		///< Write dummy data for shader modules
		const uint64_t shaderModuleIndexPos = serializer.GetStreamPosition();
		serializer.WriteZero(shaderPackFile.Header.ShaderModuleCount * sizeof(ShaderPackFile::ShaderModuleInfo));
		for (const auto &shader : shaderMap | std::views::values)
		{
			const Ref<Shader> vulkanShader = shader.As<Shader>();

			///< Serialize reflection data
			shaderPackFile.Index.ShaderPrograms[(uint32_t)vulkanShader->GetHash()].ReflectionDataOffset = serializer.GetStreamPosition();
			vulkanShader->SerializeReflectionData(&serializer);

			///< Serialize SPIR-V data
            for (const auto& shaderData = vulkanShader->m_ShaderData; const auto& [stage, data] : shaderData)
			{
				auto& indexShaderModule = shaderPackFile.Index.ShaderModules.emplace_back();
				indexShaderModule.PackedOffset = serializer.GetStreamPosition();
				indexShaderModule.PackedSize = data.size();
				indexShaderModule.Stage = (uint8_t)Utils::ShaderStageFromVkShaderStage(stage);
				serializer.WriteArray(data, false);
			}
		}

		///< Write program index
		serializer.SetStreamPosition(shaderProgramIndexPos);
		const uint64_t begin = shaderProgramIndexPos;
		for (const auto& [name, programInfo] : shaderPackFile.Index.ShaderPrograms)
		{
			serializer.WriteRaw(name);
			serializer.WriteRaw(programInfo.ReflectionDataOffset);
			serializer.WriteArray(programInfo.ModuleIndices);
		}

		const uint64_t end = serializer.GetStreamPosition();
		uint64_t s = end - begin;

		///< Write module index
		serializer.SetStreamPosition(shaderModuleIndexPos);
		serializer.WriteArray(shaderPackFile.Index.ShaderModules, false);
		return shaderPack;
	}

#if 0
	void ShaderPack::CompileAndStoreShader(std::string_view shaderSourcePath)
	{
		uint32_t hash = Hash::GenerateFNVHash(shaderSourcePath);
		m_File.Index.ShaderPrograms[hash];
		BinarySerializer serializer(m_File);
		serializer.SerializeToFile(m_Path);
	}
	void ShaderPack::AddShader(Ref<Shader> shader)
	{
	}
#endif
*/

}

/// -------------------------------------------------------
