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
#include <array>
#include <filesystem>
#include <utility>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang.h>
#include <spirv_cross/spirv_cross.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
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
		SEDX_VK_RESULT_ASSERT(vkCreateShaderModule(device->GetLogicalDevice(), &createInfo, nullptr, &module),
			"Failed to create shader module");
		return module;
	}

    ShaderManager::ShaderManager(const void *spirvCode, size_t codeSize)
    {
        // Use the same module for vertex and fragment stages by default.
        m_Stages.reserve(2);
        m_Modules.reserve(2);
        m_Stages.push_back(VK_SHADER_STAGE_VERTEX_BIT);
        m_Modules.push_back(CreateShaderModule(spirvCode, codeSize));
        m_Stages.push_back(VK_SHADER_STAGE_FRAGMENT_BIT);
        m_Modules.push_back(CreateShaderModule(spirvCode, codeSize));
    }

    ShaderManager::ShaderManager(
        const std::vector<std::pair<VkShaderStageFlagBits, std::pair<const void *, size_t>>> &stages)
    {
        m_Stages.reserve(stages.size());
        m_Modules.reserve(stages.size());
        for (const auto &s : stages)
        {
            m_Stages.push_back(s.first);
            m_Modules.push_back(CreateShaderModule(s.second.first, s.second.second));
        }
    }

    ShaderManager::~ShaderManager()
    {
        const Ref<Device> device = RenderContext::Get()->GetDevice();
        if (!device.IsValid())
        {
            return;
        }

        for (VkShaderModule module : m_Modules)
        {
            if (module != VK_NULL_HANDLE)
            {
                vkDestroyShaderModule(device->GetLogicalDevice(), module, nullptr);
            }
        }
        m_Modules.clear();
    }
	
	Ref<Shader> &ShaderManager::CreateShader(const std::string &name)
    {
        if (m_Shaders.contains(name))
        {
            return m_Shaders[name];
        }

        m_Shaders[name] = CreateRef<Shader>();
        return m_Shaders[name];
    }
	
	Ref<Shader> &ShaderManager::GetShader(const std::string &name)
    {
        SEDX_CORE_ASSERT(m_Shaders.contains(name), "Shader %s is not present", name.c_str());
        return m_Shaders[name];
    }
	
	void ShaderManager::Clear()
    {
        m_Shaders.clear();
    }


} // namespace SceneryEditorX

#pragma region Slang Compilation and Reflection

namespace ShaderCompiler
{
	std::vector<uint32_t> CompileVulkanShader(SceneryEditorX::Stage stage, const std::string& filepath, bool optimize)
	{
     (void)stage;
		(void)optimize;

		Slang::ComPtr<slang::IGlobalSession> globalSession;
		if (SLANG_FAILED(slang::createGlobalSession(globalSession.writeRef())))
		{
          SEDX_CORE_ERROR_TAG("Shader", "Failed to create Slang global session");
			return std::vector<uint32_t>();
		}

		auto targets = std::to_array<slang::TargetDesc>({
			{.format = SLANG_SPIRV, .profile = globalSession->findProfile("spirv_1_4")}
		});

		auto options = std::to_array<slang::CompilerOptionEntry>({
			{.name = slang::CompilerOptionName::EmitSpirvDirectly,
			 .value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1}}
		});

		slang::SessionDesc sessionDesc{};
		sessionDesc.targets = targets.data();
		sessionDesc.targetCount = static_cast<SlangInt>(targets.size());
		sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
		sessionDesc.compilerOptionEntries = options.data();
		sessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(options.size());

		Slang::ComPtr<slang::ISession> session;
		if (SLANG_FAILED(globalSession->createSession(sessionDesc, session.writeRef())))
		{
			SEDX_CORE_ERROR_TAG("Shader", "Failed to create Slang session");
			return std::vector<uint32_t>();
		}

        Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		const std::filesystem::path shaderPath(filepath);
		const std::string moduleName = shaderPath.stem().string();

        Slang::ComPtr<slang::IModule> module{session->loadModuleFromSource(moduleName.c_str(), filepath.c_str(), nullptr, diagnosticsBlob.writeRef())};
		if (!module)
		{
			if (diagnosticsBlob)
			{
               SEDX_CORE_ERROR_TAG("Shader", "Slang compilation failed: {}", static_cast<const char*>(diagnosticsBlob->getBufferPointer()));
			}
			return std::vector<uint32_t>();
		}

		Slang::ComPtr<ISlangBlob> spirv;
		if (SLANG_FAILED(module->getTargetCode(0, spirv.writeRef())) || !spirv)
		{
			SEDX_CORE_ERROR_TAG("Shader", "Failed to retrieve SPIR-V from Slang module: {}", filepath);
			return std::vector<uint32_t>();
		}

		const size_t byteSize = spirv->getBufferSize();
		if (byteSize == 0 || (byteSize % sizeof(uint32_t)) != 0)
		{
			SEDX_CORE_ERROR_TAG("Shader", "Invalid SPIR-V blob size from Slang for {}", filepath);
			return std::vector<uint32_t>();
		}

		std::vector<uint32_t> byteCode(byteSize / sizeof(uint32_t));
		std::memcpy(byteCode.data(), spirv->getBufferPointer(), byteSize);
		return byteCode;
	}
	
    std::vector<SceneryEditorX::ShaderInput> Reflect(SceneryEditorX::Stage stage, const std::vector<uint32_t>& shaderBytecode)
	{

        /*
        std::vector<SceneryEditorX::ShaderInput> shaderInput;
	
		spirv_cross::Compiler compiler(shaderBytecode);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();
	
		// Uniform buffers
		for (const spirv_cross::Resource& uniformBuffer : resources.uniform_buffers)
		{
            SceneryEditorX::ShaderInput uniformBufferInput = {};
            uniformBufferInput.stage = stage;
			uniformBufferInput.debugName = uniformBuffer.name;
			uniformBufferInput.set = compiler.get_decoration(uniformBuffer.id, spv::DecorationDescriptorSet);
			uniformBufferInput.binding = compiler.get_decoration(uniformBuffer.id, spv::DecorationBinding);
			uniformBufferInput.count = compiler.get_type(uniformBuffer.type_id).array[0] == 0 ? 1 : compiler.get_type(uniformBuffer.type_id).array[0];
            uniformBufferInput.type = SceneryEditorX::ShaderInputType::UniformBuffer;
	
			shaderInput.push_back(uniformBufferInput);
		}
	
		// Samplers
		for (const spirv_cross::Resource& sampler : resources.sampled_images)
		{
            SceneryEditorX::ShaderInput sampleImageInput = {};
            sampleImageInput.stage = stage;
			sampleImageInput.debugName = sampler.name;
			sampleImageInput.set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			sampleImageInput.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			sampleImageInput.count = compiler.get_type(sampler.type_id).array[0] == 0 ? 1 : compiler.get_type(sampler.type_id).array[0];
            sampleImageInput.type = SceneryEditorX::ShaderInputType::CombinedImageSampler;
	
			shaderInput.push_back(sampleImageInput);
		}
	
		for (const spirv_cross::Resource& texture : resources.separate_images)
		{
			const spirv_cross::SPIRType& spirType = compiler.get_type(texture.type_id);
	
            SceneryEditorX::ShaderInput sampleImageInput = {};
            sampleImageInput.stage = stage;
			sampleImageInput.debugName = texture.name;
			sampleImageInput.set = compiler.get_decoration(texture.id, spv::DecorationDescriptorSet);
			sampleImageInput.binding = compiler.get_decoration(texture.id, spv::DecorationBinding);
			sampleImageInput.count = spirType.array.empty() ? 1 : spirType.array[0];
            sampleImageInput.type = SceneryEditorX::ShaderInputType::Texture;
	
			shaderInput.push_back(sampleImageInput);
		}
	
		for (const spirv_cross::Resource& sampler : resources.separate_samplers)
		{
            SceneryEditorX::ShaderInput sampleImageInput = {};
            sampleImageInput.stage = stage;
			sampleImageInput.debugName = sampler.name;
			sampleImageInput.set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			sampleImageInput.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			sampleImageInput.count = compiler.get_type(sampler.type_id).array[0] == 0 ? 1 : compiler.get_type(sampler.type_id).array[0];
            sampleImageInput.type = SceneryEditorX::ShaderInputType::Sampler;
	
			shaderInput.push_back(sampleImageInput);
		}
	
		for (const spirv_cross::Resource& sampler : resources.storage_images)
		{
            SceneryEditorX::ShaderInput sampleImageInput = {};
            sampleImageInput.stage = stage;
			sampleImageInput.debugName = sampler.name;
			sampleImageInput.set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			sampleImageInput.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			sampleImageInput.count = compiler.get_type(sampler.type_id).array[0] == 0 ? 1 : compiler.get_type(sampler.type_id).array[0];
            sampleImageInput.type = SceneryEditorX::ShaderInputType::StorageImage;
	
			shaderInput.push_back(sampleImageInput);
		}
	
		for (const spirv_cross::Resource& storageBuffer : resources.storage_buffers)
		{
            SceneryEditorX::ShaderInput storageBufferInput = {};
            storageBufferInput.stage = stage;
			storageBufferInput.debugName = storageBuffer.name;
			storageBufferInput.set = compiler.get_decoration(storageBuffer.id, spv::DecorationDescriptorSet);
			storageBufferInput.binding = compiler.get_decoration(storageBuffer.id, spv::DecorationBinding);
			storageBufferInput.count = compiler.get_type(storageBuffer.type_id).array[0] == 0 ? 1 : compiler.get_type(storageBuffer.type_id).array[0];
            storageBufferInput.type = SceneryEditorX::ShaderInputType::StorageBuffer;
	
			shaderInput.push_back(storageBufferInput);
		}
	
        return shaderInput;
        */

	    (void)stage;
		(void)shaderBytecode;
		SEDX_CORE_WARN_TAG("Shader", "SPIR-V reflection disabled: SPIRV-Cross backend not linked.");
		return {};
	}

}

#pragma endregion

// -------------------------------------------------------
