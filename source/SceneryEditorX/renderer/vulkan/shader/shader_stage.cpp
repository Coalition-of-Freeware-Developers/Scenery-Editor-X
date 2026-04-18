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
#include <array>
#include <filesystem>
#include <fstream>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @brief Determines the default entry point for a shader based on its stage and file extension. 
	 * @param stage The shader stage.
	 * @param filepath The path to the shader file.
	 * @return The default entry point for the shader.
	 */
	static const char* GetDefaultEntryPoint(const StageType stage, const std::string& filepath)
	{
		if (const bool isSlangShader = filepath.ends_with(".slang"); !isSlangShader)
		{
			return "main";
		}

		switch (stage)
		{
			case StageType::Vertex:					return "main_vs";
			case StageType::Fragment:				return "main_frag";
			case StageType::Compute:				return "main_comp";
			case StageType::Geometry:				return "main_geo";
			case StageType::TessellationControl:	return "main_tesc";
			case StageType::TessellationEvaluation:	return "main_tese";
			default:								return "main";
		}
	}

	/**
	 * @brief Resolves the absolute path of a shader source file.
	 * @param filepath The relative or absolute path to the shader file.
	 * @return The resolved absolute path to the shader file.
	 */
	static std::filesystem::path ResolveSourcePath(const std::string& filepath)
	{
		const std::filesystem::path inputPath(filepath);
		if (inputPath.is_absolute())
		{
			return inputPath;
		}

		const std::filesystem::path cwd = std::filesystem::current_path();
		const std::array<std::filesystem::path, 6> candidates = {
			cwd / inputPath,
			cwd / "resources" / "shaders" / inputPath,
			cwd / ".." / inputPath,
			cwd / ".." / "resources" / "shaders" / inputPath,
			cwd / ".." / ".." / inputPath,
			cwd / ".." / ".." / "resources" / "shaders" / inputPath
		};

		for (const auto& candidate : candidates)
		{
			if (std::filesystem::exists(candidate))
			{
				return std::filesystem::weakly_canonical(candidate);
			}
		}

		return candidates[0];
	}

	/**
	 * @brief Returns the file suffix for a given shader stage.
	 * @param stage The shader stage.
	 * @return The file suffix for the shader stage.
	 */
	static std::string StageSuffix(const StageType stage)
	{
		switch (stage)
		{
			case StageType::Vertex:					return "vert";
			case StageType::Fragment:				return "frag";
			case StageType::Compute:				return "comp";
			case StageType::Geometry:				return "geom";
			case StageType::TessellationControl:	return "tesc";
			case StageType::TessellationEvaluation:	return "tese";
			default:								return "unknown";
		}
	}

	/**
	 * @brief Returns the cache path for a given shader stage and source path.
	 * @param stage The shader stage.
	 * @param sourcePath The path to the shader source file.
	 * @return The cache path for the shader stage.
	 */
	static std::filesystem::path GetCachePath(const StageType stage, const std::filesystem::path& sourcePath)
	{
		const auto &context = Application::Get().GetPlatformContext();
		const auto &appDir = context->GetTempDirectory();
		std::filesystem::path cacheRoot = appDir + "SceneryEditorX\\shader-cache";
		const std::filesystem::path fileStem = sourcePath.stem();
		const std::string cacheName = fileStem.string() + "." + StageSuffix(stage) + ".spv";
		return cacheRoot / cacheName;
	}

	/**
	 * @brief Reads the cached shader data from a file.
	 * @param cachePath The path to the cached shader file.
	 * @return A vector containing the cached shader data.
	 */
	static std::vector<uint32_t> ReadCachedShaderData(const std::filesystem::path& cachePath)
	{
		std::ifstream input(cachePath, std::ios::binary | std::ios::ate);
		if (!input.is_open())
		{
			return {};
		}

		const std::streamsize size = input.tellg();
		if (size <= 0 || (size % static_cast<std::streamsize>(sizeof(uint32_t))) != 0)
		{
			return {};
		}

		std::vector<uint32_t> result(static_cast<size_t>(size / static_cast<std::streamsize>(sizeof(uint32_t))));
		input.seekg(0, std::ios::beg);
		if (!input.read(reinterpret_cast<char*>(result.data()), size))
		{
			return {};
		}

		return result;
	}

	/**
	 * @brief Writes the shader binary data to a cache file.
	 * @param data The shader binary data to write.
	 * @param cachePath The path to the cache file.
	 * @return True if the shader binary was successfully written, false otherwise.
	 */
	static bool WriteShaderBinary(const std::vector<uint32_t>& data, const std::filesystem::path& cachePath)
	{
		if (data.empty())
			return false;

		std::filesystem::create_directories(cachePath.parent_path());
		std::ofstream output(cachePath, std::ios::binary | std::ios::trunc);
		if (!output.is_open())
			return false;

		const std::streamsize byteCount = static_cast<std::streamsize>(data.size() * sizeof(uint32_t));
		output.write(reinterpret_cast<const char*>(data.data()), byteCount);
		return output.good();
	}
	
	// -------------------------------------------------------

	ShaderStage::ShaderStage(StageType stage, const std::string& filepath) : m_Stage(stage), m_Filepath(filepath)
	{
		m_EntryPoint = GetDefaultEntryPoint(stage, filepath);
		const bool built = BuildOrRebuildModule();
		SEDX_CORE_ASSERT(built, "Failed to build shader stage module for '{}'", filepath);
	}
	
	ShaderStage::~ShaderStage()
	{
	   DestroyModule();
	}
	
	void ShaderStage::Recompile()
	{
		const bool rebuilt = BuildOrRebuildModule();
		SEDX_CORE_ASSERT(rebuilt, "Failed to recompile shader stage '{}'", m_Filepath);
	}

	bool ShaderStage::BuildOrRebuildModule(bool optimize)
	{
		const Ref<RenderContext> context = RenderContext::Get();
		if (!context.IsValid())
		{
			SEDX_CORE_ERROR_TAG("Shader", "Cannot build shader stage '{}' without a valid render context", m_Filepath);
			return false;
		}

		const Ref<Device> device = context->GetDevice();
		if (!device.IsValid())
		{
			SEDX_CORE_ERROR_TAG("Shader", "Cannot build shader stage '{}' without a valid device", m_Filepath);
			return false;
		}

		const std::filesystem::path sourcePath = ResolveSourcePath(m_Filepath);
		const std::filesystem::path cachePath = GetCachePath(m_Stage, sourcePath);

		std::vector<uint32_t> data;
		bool shouldCompile = true;

		if (std::filesystem::exists(cachePath) && std::filesystem::exists(sourcePath))
		{
			const auto sourceWrite = std::filesystem::last_write_time(sourcePath);
			const auto cacheWrite = std::filesystem::last_write_time(cachePath);
			shouldCompile = sourceWrite > cacheWrite;
		}

		if (!shouldCompile)
		{
			data = ReadCachedShaderData(cachePath);
			shouldCompile = data.empty();
		}

		if (shouldCompile)
		{
			SEDX_CORE_TRACE_TAG("Shader", "Compiling stage '{}' from '{}'", StageSuffix(m_Stage), sourcePath.string());
			data = ShaderCompiler::CompileShader(m_Stage, sourcePath.string(), optimize);
			if (!data.empty())
			{
				WriteShaderBinary(data, cachePath);
			}
		}

		if (data.empty())
		{
			SEDX_CORE_ERROR_TAG("Shader", "Shader compile/load produced empty bytecode for '{}'", sourcePath.string());
			return false;
		}

		m_Input = ShaderCompiler::Reflect(m_Stage, data);

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = data.size() * sizeof(uint32_t);
		createInfo.pCode = data.data();

		VkShaderModule newModule = VK_NULL_HANDLE;
		if (vkCreateShaderModule(device->GetLogicalDevice(), &createInfo, nullptr, &newModule) != VK_SUCCESS)
		{
			SEDX_CORE_ERROR_TAG("Shader", "Failed to create VkShaderModule for '{}'", sourcePath.string());
			return false;
		}

		DestroyModule();
		m_ShaderModule = newModule;
		return true;
	}

	void ShaderStage::DestroyModule()
	{
		if (m_ShaderModule == VK_NULL_HANDLE)
			return;

		const Ref<RenderContext> context = RenderContext::Get();
		if (!context.IsValid())
		{
			m_ShaderModule = VK_NULL_HANDLE;
			return;
		}

		const Ref<Device> device = context->GetDevice();
		if (!device.IsValid())
		{
			m_ShaderModule = VK_NULL_HANDLE;
			return;
		}

		vkDestroyShaderModule(device->GetLogicalDevice(), m_ShaderModule, nullptr);
		m_ShaderModule = VK_NULL_HANDLE;
	}

	VkPipelineShaderStageCreateInfo ShaderStage::GetStageCreateInfo() const
	{
		VkPipelineShaderStageCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfo.stage = GetStage(m_Stage);
		createInfo.module = m_ShaderModule;
		createInfo.pName = m_EntryPoint.c_str();
	
		return createInfo;
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
