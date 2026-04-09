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
// ReSharper disable CppConstValueFunctionReturnType
#include "shader_stage.h"
#include "shader_manager.h"
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @brief Returns the default entry point for a given shader stage and file path.
	 * @param stage The shader stage.
	 * @param filepath The path to the shader file.
	 * @return The default entry point name.
	 */
	static const char* GetDefaultEntryPoint(const Stage stage, const std::string& filepath)
	{
		const bool isSlangShader = filepath.ends_with(".slang");
		if (!isSlangShader)
		{
			return "main";
		}

		switch (stage)
		{
			case Stage::Vertex:					return "main_vs";
			case Stage::Fragment:				return "main_frag";
			case Stage::Compute:				return "main_comp";
			case Stage::Geometry:				return "main_geo";
			case Stage::TessellationControl:	return "main_tcs";
			case Stage::TessellationEvaluation:	return "main_tes";
			default:							return "main";
		}
	}
	
	#define SOURCE_FILEPATH "resources/shaders/"
	#define CACHE_FILEPATH "cache"
	
	// -------------------------------------------------------

	/**
	 * @brief Reads cached SPIR-V binary data from a file and returns it as a vector of uint32_t.
	 * @param filepath The path to the cached SPIR-V file.
	 * @return A vector containing the SPIR-V binary data.
	 */
	static std::vector<uint32_t> ReadCachedShaderData(const std::string& filepath)
	{
		FILE* f;
		fopen_s(&f, filepath.c_str(), "rb");
		SEDX_CORE_ASSERT(!f, "Failed to open shader cache file: {}", filepath);
	
		fseek(f, 0, SEEK_END);
		uint64_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
	
		std::vector<uint32_t> buffer(size / sizeof(uint32_t));
		fread(buffer.data(), sizeof(uint32_t), buffer.size(), f);
	
		fclose(f);
	
		return buffer;
	}
	
	/**
	 * @brief Writes SPIR-V binary data to a file.
	 * @param data Pointer to the SPIR-V binary data.
	 * @param size Size of the SPIR-V binary data in bytes.
	 * @param path The path to the output file.
	 */
	static void WriteShaderBinary(const void* data, uint32_t size, const std::string& path)
	{
		FILE* file;
		fopen_s(&file, path.c_str(), "wb");
		SEDX_CORE_ASSERT(file, "Failed to open file: {}", path);

		fwrite(data, sizeof(uint32_t), size, file);
		fclose(file);
	}
	
	// -------------------------------------------------------

	ShaderStage::ShaderStage(Stage stage, const std::string& filepath) : m_Stage(stage), m_Filepath(filepath)
	{
		m_EntryPoint = GetDefaultEntryPoint(stage, filepath);

		const Ref<Device> device = RenderContext::Get()->GetDevice();

		size_t lastD = filepath.find_last_of('/');
		std::string directoryPath = std::string(filepath.begin(), filepath.begin() + (lastD != std::string::npos ? lastD : 0));
		std::string shaderName = std::string(filepath.begin(), filepath.begin() + filepath.find_last_of('.'));

		/**
		 * If the provided filepath already references the resources directory (or is absolute),
		 * don't prefix it with SOURCE_FILEPATH to avoid duplicated "resources/shaders/resources/..." paths.
		 */
		std::string codeFilepath;
		const bool filepathStartsWithResources = (filepath.starts_with("resources")) || (filepath.starts_with("resources\\"));
		const bool filepathLooksAbsolute = !filepath.empty() && (filepath[0] == '/' || (filepath.size() > 1 && filepath[1] == ':'));

		if (filepathStartsWithResources || filepathLooksAbsolute)
		{
			codeFilepath = filepath;
		}
		else
		{
			codeFilepath = std::string(SOURCE_FILEPATH) + filepath;
		}

		// Ensure the .spv cache subdirectory exists
		std::filesystem::path cacheDir    = std::filesystem::path(CACHE_FILEPATH);
		std::filesystem::path cacheSubDir = cacheDir / directoryPath;
		if (!std::filesystem::exists(cacheSubDir))
		{
			std::filesystem::create_directories(cacheSubDir);
		}

		// Compile via Slang and reflect in a single pass.
		// LoadOrCompileModule inside CompileAndReflect checks the .slang-module cache so
		// the Slang front-end is only invoked the first time (or when the source changes).
		SEDX_CORE_TRACE_TAG("Shader", "Compiling/loading shader: {}", codeFilepath);
		const ShaderCompiler::ShaderCompilationResult result = ShaderCompiler::CompileAndReflect(stage, codeFilepath);

		m_Input = result.inputs;

		if (result.spirv.empty())
		{
			SEDX_CORE_ERROR_TAG("Shader", "Failed to compile shader: {}", codeFilepath.c_str());
			SEDX_CORE_ASSERT(false, "Failed to compile shader: {}", codeFilepath.c_str());
			return;
		}

		// Write the SPIR-V words to a .spv artifact alongside the .slang-module cache
		const std::filesystem::path spvCache = cacheDir / (shaderName + ".spv");
		WriteShaderBinary(result.spirv.data(), static_cast<uint32_t>(result.spirv.size()), spvCache.string());

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = result.spirv.size() * sizeof(uint32_t);
		createInfo.pCode    = result.spirv.data();

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

		const std::string codeFilepath = SOURCE_FILEPATH + m_Filepath;
		const ShaderCompiler::ShaderCompilationResult result = ShaderCompiler::CompileAndReflect(m_Stage, codeFilepath);

		if (result.spirv.empty())
		{
			SEDX_CORE_ERROR_TAG("Shader", "Recompile failed for: {}", codeFilepath);
			return;
		}

		const std::string shaderName = std::string(m_Filepath.begin(), m_Filepath.begin() + m_Filepath.find_last_of('.'));
		const std::string spvCache   = std::string(CACHE_FILEPATH) + shaderName + ".spv";
		WriteShaderBinary(result.spirv.data(), static_cast<uint32_t>(result.spirv.size()), spvCache);

		m_Input = result.inputs;

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = result.spirv.size() * sizeof(uint32_t);
		createInfo.pCode    = result.spirv.data();

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
	  createInfo.pName = m_EntryPoint.c_str();
	
		return createInfo;
	}

	
}

// -------------------------------------------------------
