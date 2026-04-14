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
 * shader_manager.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "shader_manager.h"
#include "shader_compiler.h"
#include <utility>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang.h>
#include <spirv_cross/spirv_cross.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	static Ref<ShaderManager> s_Instance = nullptr;
	std::mutex ShaderManager::s_ShaderMutex;
	std::unordered_map<std::string, Ref<Shader>> ShaderManager::m_Shaders;

	static VkShaderModule CreateShaderModule(const void* spirvCode, size_t codeSize)
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		SEDX_CORE_ASSERT(device.IsValid(), "Invalid device while creating shader module");
		SEDX_CORE_ASSERT(spirvCode != nullptr && codeSize > 0, "Invalid SPIR-V input for shader module creation");

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = codeSize;
		createInfo.pCode = reinterpret_cast<const uint32_t*>(spirvCode);

		VkShaderModule module = VK_NULL_HANDLE;
		SEDX_VK_RESULT_ASSERT(vkCreateShaderModule(device->GetLogicalDevice(), &createInfo, nullptr, &module), "Failed to create shader module");
		return module;
	}

	void ShaderManager::CreateSingleBlob(const void *spirvCode, size_t codeSize)
	{
		// Use the same module for vertex and fragment stages by default.
		m_Stages.reserve(2);
		m_Modules.reserve(2);
		m_Stages.push_back(VK_SHADER_STAGE_VERTEX_BIT);
		m_Modules.push_back(CreateShaderModule(spirvCode, codeSize));
		m_Stages.push_back(VK_SHADER_STAGE_FRAGMENT_BIT);
		m_Modules.push_back(CreateShaderModule(spirvCode, codeSize));

		// Mark as compiled only if all modules were created successfully
		const bool allValid = std::ranges::all_of(m_Modules, [](VkShaderModule m) { return m != VK_NULL_HANDLE; });
		/**
		 * Note: compilation state is tracked per-Shader (SceneryEditorX::Shader). ShaderManager does not
		 * own a m_CompilationState for individual shaders; per-shader state should be updated on creation
		 * or reload via Shader instances. Leave this constructor responsibility to the owning Shader objects.
		 */
	}

	void ShaderManager::CreateShaderBlobs(const std::vector<std::pair<VkShaderStageFlagBits, std::pair<const void *, size_t>>> &stages)
	{
		m_Stages.reserve(stages.size());
		m_Modules.reserve(stages.size());
		for (const auto &s : stages)
		{
			m_Stages.push_back(s.first);
			m_Modules.push_back(CreateShaderModule(s.second.first, s.second.second));
		}

		// Mark as compiled only if all modules were created successfully
		// Note: Do NOT attempt to set a Shader-specific compilation state from here.
		const bool allValid = !m_Modules.empty() && std::ranges::all_of(m_Modules, [](VkShaderModule m) { return m != VK_NULL_HANDLE; });
	}

	ShaderManager::ShaderManager()
	{
		SEDX_CORE_TRACE_TAG("ShaderManager", "Created ShaderManager");
	}

	ShaderManager::~ShaderManager()
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		if (!device.IsValid())
			return;

		for (VkShaderModule module : m_Modules)
		{
			if (module != VK_NULL_HANDLE)
			{
				vkDestroyShaderModule(device->GetLogicalDevice(), module, nullptr);
			}
		}
		m_Modules.clear();
	}

	Ref<ShaderManager> ShaderManager::Get()
	{
		if (!s_Instance)
			s_Instance = CreateRef<ShaderManager>();

		return s_Instance;
	}

	Ref<Shader> &ShaderManager::CreateShader(const std::string &name)
	{
		std::scoped_lock lock(s_ShaderMutex);
		if (m_Shaders.contains(name))
			return m_Shaders[name];

		m_Shaders[name] = CreateRef<Shader>(name.c_str());
		return m_Shaders[name];
	}

	Ref<Shader> &ShaderManager::CreateShader(const std::string& name, const std::string &path, bool forceCompile)
	{
		std::scoped_lock lock(s_ShaderMutex);
		if (m_Shaders.contains(name))
			return m_Shaders[name];

		m_Shaders[name] = CreateRef<Shader>(name.c_str(), path, forceCompile);
		return m_Shaders[name];
	}

	Ref<Shader> &ShaderManager::GetShader(const std::string &name)
	{
		std::scoped_lock lock(s_ShaderMutex);
		SEDX_CORE_ASSERT(m_Shaders.contains(name), "Shader {} is not present", name.c_str());
		return m_Shaders[name];
	}

	void ShaderManager::ReloadShader(const Ref<Shader> &shaderName, bool forceCompile)
	{
		SEDX_CORE_ASSERT(shaderName.IsValid(), "Invalid shader reference");
		SEDX_CORE_WARN_TAG("ShaderManager", "Shader Reloading is not implemented yet");
		/*
		Renderer::Submit([shaderName, forceCompile]()
		{
		    RenderThread_Reload(shaderName, forceCompile);
		});
		*/
	}

	void ShaderManager::RenderThread_Reload(const Ref<Shader> &shaderName, bool forceCompile)
	{
		SEDX_CORE_ASSERT(shaderName.IsValid(), "Invalid shader reference");

		// Iterate over all possible StageType enum values and recompile any stages present
		for (int i = 0; i < static_cast<int>(StageType::MaxEnum); ++i)
		{
			const StageType stage = static_cast<StageType>(i);
			if (!shaderName->HasStage(stage))
				continue; 

			Ref<ShaderStage> shaderStage = shaderName->GetShaderStage(stage);
			SEDX_CORE_ASSERT(shaderStage.IsValid(), "ShaderStage for stage {} is invalid", i);

			// Recompile the individual shader stage which should update its SPIR-V and VkShaderModule
			shaderStage->Recompile();
		}

		// Individual ShaderStage::Recompile handles the stage compilation and module recreation.
	}
		
	void ShaderManager::ClearAll()
	{
		std::scoped_lock lock(s_ShaderMutex);
		m_Shaders.clear();
	}

	void ShaderManager::ClearStage(const Ref<Shader> &shader, const StageType stage)
	{

	}

	std::vector<uint32_t> ShaderManager::CompileToSpirv(const StageType stage, const std::string &filepath, const bool optimize)
	{
		/* TODO: Do cache lookups here... */

		return ShaderCompiler::CompileShader(stage, filepath, optimize);
	}

	std::vector<ShaderInput> ShaderManager::ReflectInputs(const StageType stage, const std::vector<uint32_t> &spirv)
	{
		return ShaderCompiler::Reflect(stage, spirv);
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
