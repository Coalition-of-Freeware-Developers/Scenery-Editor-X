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
* shader_stage.cpp
* -------------------------------------------------------
* Created: 24/02/2026
* -------------------------------------------------------
*/
#include "shader_stage.h"
#include "render_context.h"
#include "shader_manager.h"
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	#define SOURCE_FILEPATH "resources/shaders/"
	#define CACHE_FILEPATH "cache/shaders/"
	
    // -------------------------------------------------------

	/*
	static std::vector<uint32_t> ReadCachedShaderData(const std::string& filepath)
	{
		FILE* f;
		fopen_s(&f, filepath.c_str(), "rb");
	
		if (!f)
		{
			std::cerr << "Fisierul " << filepath << " nu exista\n";
			assert(false);
		}
	
		fseek(f, 0, SEEK_END);
		uint64_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
	
		std::vector<uint32_t> buffer(size / sizeof(uint32_t));
		fread(buffer.data(), sizeof(uint32_t), buffer.size(), f);
	
		fclose(f);
	
		return buffer;
	}
	
	static void WriteShaderBinary(void* data, uint32_t size, const std::string& path)
	{
		FILE* file;
		fopen_s(&file, path.c_str(), "wb");
		if (!file)
		{
		    assert(false);
		}
		fwrite(data, sizeof(uint32_t), size, file);
		fclose(file);
	}
	
    // -------------------------------------------------------

	ShaderStage::ShaderStage(Stage stage, const std::string& filepath) : m_Stage(stage), m_Filepath(filepath)
	{
        const Ref<Device> device = RenderContext::Get()->GetDevice();
		std::vector<uint32_t> data;
	
		size_t lastD = filepath.find_last_of('/');
		std::string directoryPath = std::string(filepath.begin(), filepath.begin() + (lastD != std::string::npos ? lastD : 0));
		std::string shaderName = std::string(filepath.begin(), filepath.begin() + filepath.find_last_of('.'));
	
		std::string codeFilepath = SOURCE_FILEPATH + filepath;
		std::string cacheFilepath = CACHE_FILEPATH + shaderName + ".spv";
	
		bool shouldRecompile = false;
	
		if (!std::filesystem::exists(cacheFilepath))
			shouldRecompile = true;
		else
		{
			std::filesystem::file_time_type lastModifiedCache = std::filesystem::last_write_time(cacheFilepath);
			std::filesystem::file_time_type lastModifiedShader = std::filesystem::last_write_time(codeFilepath);
			shouldRecompile = lastModifiedShader > lastModifiedCache;
		}
	
		if (!std::filesystem::exists(CACHE_FILEPATH + directoryPath))
			std::filesystem::create_directories(CACHE_FILEPATH + directoryPath);
	
		if (std::filesystem::exists(cacheFilepath) && !shouldRecompile)
		{
            SEDX_CORE_TRACE_TAG("Shader", "Loading shader data from cache: %s", cacheFilepath.c_str());
			data = ReadCachedShaderData(cacheFilepath);
		}
		else
		{
			SEDX_CORE_TRACE_TAG("Shader", "Compiling shader: %s", cacheFilepath.c_str());
			data = ShaderCompiler::CompileVulkanShader(stage, codeFilepath);
			WriteShaderBinary(data.data(), (uint32_t)data.size(), cacheFilepath);
		}
	
		m_Input = ShaderCompiler::Reflect(stage, data);
	
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = data.size() * sizeof(uint32_t);
		createInfo.pCode = data.data();
	
		SEDX_VK_RESULT_ASSERT(vkCreateShaderModule(device->GetLogicalDevice(), &createInfo, nullptr, &m_ShaderModule), "Can't create shader module")
	}
	
	ShaderStage::~ShaderStage()
	{
        const Ref<Device> device = RenderContext::Get()->GetDevice();
		SEDX_CORE_ASSERT(device.IsValid(), "Invalid device");

		vkDestroyShaderModule(device->GetLogicalDevice(), m_ShaderModule, nullptr);	
	}
	
	void ShaderStage::Recompile()
	{
	    const Ref<Device> device = RenderContext::Get()->GetDevice();
		std::vector<uint32_t> data = ShaderCompiler::CompileVulkanShader(m_Stage, SOURCE_FILEPATH + m_Filepath);
	
		std::string shaderName = std::string(m_Filepath.begin(), m_Filepath.begin() + m_Filepath.find_last_of('.'));
		std::string cacheFilepath = CACHE_FILEPATH + shaderName + ".spv";
		WriteShaderBinary(data.data(), (uint32_t)data.size(), cacheFilepath);
	
		m_Input = ShaderCompiler::Reflect(m_Stage, data);
	
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = data.size() * sizeof(uint32_t);
		createInfo.pCode = data.data();
	
		vkDestroyShaderModule(device->GetLogicalDevice(), m_ShaderModule, nullptr);
		m_ShaderModule = VK_NULL_HANDLE;
	
		SEDX_VK_RESULT_ASSERT(vkCreateShaderModule(device->GetLogicalDevice(), &createInfo, nullptr, &m_ShaderModule), "Can't create shader module");
	}
	
	VkPipelineShaderStageCreateInfo const ShaderStage::GetStageCreateInfo()
	{
		VkPipelineShaderStageCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfo.stage = GetStage(m_Stage);
		createInfo.module = m_ShaderModule;
		createInfo.pName = "main";
	
		return createInfo;
	}
	*/
	
}

// -------------------------------------------------------
