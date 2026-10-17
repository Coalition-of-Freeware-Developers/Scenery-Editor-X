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
 * shader_pack.h
 * -------------------------------------------------------
 * Created: 03/07/2026
 * -------------------------------------------------------
 */
#include "shader_library.h"
#include "shader_pack.h"
#include <SceneryEditorX/core/identifiers/hash.h>
#include <SceneryEditorX/utils/filestreaming/file_streaming.h>
#include <SceneryEditorX/utils/pointers.h>
#include <map>

// -------------------------------------------------------

namespace SceneryEditorX 
{

	ShaderPack::ShaderPack(const std::filesystem::path& path) : m_Path(path) 
	{
		// Read index
		FileStreamReader serializer(path);
		if (!serializer)
			return;

		serializer.ReadRaw(m_File.header);
		const ShaderPackFile::FileHeader expectedHeader{};
		if (memcmp(m_File.header.HEADER, expectedHeader.HEADER, sizeof(expectedHeader.HEADER)) != 0)
			return;

		m_Loaded = true;
		for (uint32_t i = 0; i < m_File.header.shaderProgramCount; i++)
		{
			uint32_t key;
			serializer.ReadRaw(key);
			auto& shaderProgramInfo = m_File.index.shaderPrograms[key];
			serializer.ReadRaw(shaderProgramInfo.reflectionDataOffset);
			serializer.ReadArray(shaderProgramInfo.moduleIndices);
		}

		m_File.index.shaderModules.reserve(m_File.header.shaderModuleCount);
		for (uint32_t i = 0; i < m_File.header.shaderModuleCount; ++i)
		{
			ShaderPackFile::ShaderModuleInfo info{};
			ShaderPackFile::ShaderModuleInfo::Deserialize(&serializer, info);
			m_File.index.shaderModules.push_back(info);
		}
	}

	bool ShaderPack::Contains(std::string_view name) const 
	{ 
		return m_File.index.shaderPrograms.find(Hash::GenerateFNV1A(name)) != m_File.index.shaderPrograms.end();
	}

	Ref<Shader> ShaderPack::LoadShader(std::string_view name) 
	{
		uint32_t nameHash = Hash::GenerateFNV1A(name);
		SEDX_CORE_VERIFY(Contains(name));

		const auto& shaderProgramInfo = m_File.index.shaderPrograms.at(nameHash);

		FileStreamReader serializer(m_Path);

		serializer.SetStreamPosition(shaderProgramInfo.reflectionDataOffset);
		
		// Debug only
		std::string shaderName;
		{
			std::string path(name);
			size_t found = path.find_last_of("/\\");
			shaderName = found != std::string::npos ? path.substr(found + 1) : path;
			found = shaderName.find_last_of('.');
			shaderName = found != std::string::npos ? shaderName.substr(0, found) : name;
		}

		Ref<Shader> vulkanShader = CreateRef<Shader>();
		vulkanShader->SetName(shaderName);
		vulkanShader->m_AssetPath = name;
		//vulkanShader->TryReadReflectionData(&serializer);
		// vulkanShader->m_DisableOptimization =

		std::map<ShaderType, std::vector<uint32_t>> shaderModules;
		for (uint32_t index : shaderProgramInfo.moduleIndices)
		{
			const auto& info = m_File.index.shaderModules[index];
			const ShaderType stage = static_cast<ShaderType>(info.stage);
			auto& moduleData = shaderModules[stage];

			serializer.SetStreamPosition(info.packedOffset);
			serializer.ReadArray(moduleData, (uint32_t)info.packedSize);
		}

		serializer.SetStreamPosition(shaderProgramInfo.reflectionDataOffset);
		vulkanShader->TryReadReflectionData(&serializer);

		vulkanShader->LoadAndCreateShaders(shaderModules);
		vulkanShader->CreateDescriptors();

		//Renderer::AcknowledgeParsedGlobalMacros(compiler->GetAcknowledgedMacros(), vulkanShader);
		//Renderer::OnShaderReloaded(vulkanShader->GetHash());
		return vulkanShader;
	}

	Ref<ShaderPack> ShaderPack::CreateFromLibrary(Ref<ShaderLibrary> shaderLibrary, const std::filesystem::path& path) 
	{
		Ref<ShaderPack> shaderPack = CreateRef<ShaderPack>();

		const auto& shaderMap = shaderLibrary->GetShaders();
		auto& shaderPackFile = shaderPack->m_File;
		
		shaderPackFile.header.version = 1;
		shaderPackFile.header.shaderProgramCount = (uint32_t)shaderMap.size();
		shaderPackFile.header.shaderModuleCount = 0;

		// Determine number of modules (per shader)
		// NOTE: this currently doesn't care about duplicated modules, but it should (eventually, not that important atm)
		uint32_t shaderModuleIndex = 0;
		uint32_t shaderModuleIndexArraySize = 0;
		for (const auto& [name, shader] : shaderMap)
		{
			Ref<Shader> vulkanShader = shader.As<Shader>();
			const auto& shaderData = vulkanShader->m_ShaderData;

			shaderPackFile.header.shaderModuleCount += (uint32_t)shaderData.size();
			auto& shaderProgramInfo = shaderPackFile.index.shaderPrograms[(uint32_t)vulkanShader->GetHash()];

			for (int i = 0; i < (int)shaderData.size(); i++)
				shaderProgramInfo.moduleIndices.emplace_back(shaderModuleIndex++);

			shaderModuleIndexArraySize += sizeof(uint32_t); // size
			shaderModuleIndexArraySize += (uint32_t)shaderData.size() * sizeof(uint32_t); // indices
		}

		uint32_t shaderProgramIndexSize = shaderPackFile.header.shaderProgramCount * 
			(sizeof(std::map<uint32_t, ShaderPackFile::ShaderProgramInfo>::key_type) + 
			sizeof(ShaderPackFile::ShaderProgramInfo::reflectionDataOffset)) + shaderModuleIndexArraySize;

		FileStreamWriter serializer(path);
		
		// Write header
		serializer.WriteRaw<ShaderPackFile::FileHeader>(shaderPackFile.header);

		// ===============
		// Write index
		// ===============
		// Write dummy data for shader programs
		uint64_t shaderProgramIndexPos = serializer.GetStreamPosition();
		serializer.WriteZero(shaderProgramIndexSize);

		// Write dummy data for shader modules
		uint64_t shaderModuleIndexPos = serializer.GetStreamPosition();
		serializer.WriteZero(shaderPackFile.header.shaderModuleCount * sizeof(ShaderPackFile::ShaderModuleInfo));
		for (const auto& [name, shader] : shaderMap)
		{
			Ref<Shader> vulkanShader = shader.As<Shader>();

			// Serialize reflection data
			shaderPackFile.index.shaderPrograms[(uint32_t)vulkanShader->GetHash()].reflectionDataOffset = serializer.GetStreamPosition();
			vulkanShader->SerializeReflectionData(&serializer);

			// Serialize SPIR-V data
			const auto& shaderData = vulkanShader->m_ShaderData;
			for (const auto& [stage, data] : shaderData)
			{
				auto& indexShaderModule = shaderPackFile.index.shaderModules.emplace_back();
				indexShaderModule.packedOffset = serializer.GetStreamPosition();
				indexShaderModule.packedSize = data.size();
				indexShaderModule.stage = static_cast<uint16_t>(stage);

				serializer.WriteArray(data, false);
			}
		}

		// Write program index
		serializer.SetStreamPosition(shaderProgramIndexPos);
		uint64_t begin = shaderProgramIndexPos;
		for (const auto& [name, programInfo] : shaderPackFile.index.shaderPrograms)
		{
			serializer.WriteRaw(name);
			serializer.WriteRaw(programInfo.reflectionDataOffset);
			serializer.WriteArray(programInfo.moduleIndices);
		}
		uint64_t end = serializer.GetStreamPosition();
		uint64_t s = end - begin;

		// Write module index
		serializer.SetStreamPosition(shaderModuleIndexPos);
		for (const auto& moduleInfo : shaderPackFile.index.shaderModules)
			ShaderPackFile::ShaderModuleInfo::Serialize(&serializer, moduleInfo);

		return shaderPack;
	}

#if 0
	void ShaderPack::CompileAndStoreShader(std::string_view shaderSourcePath)
	{
		uint32_t hash = Hash::GenerateFNV1A(shaderSourcePath);
		m_File.index.shaderPrograms[hash];

		BinarySerializer serializer(m_File);
		serializer.SerializeToFile(m_Path);
	}

	void ShaderPack::AddShader(Ref<Shader> shader)
	{
	}
#endif

}

// -------------------------------------------------------
